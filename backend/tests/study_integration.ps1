param([switch]$SkipBuild, [string]$BuildDir = '')
$ErrorActionPreference = 'Stop'
$backend = Split-Path -Parent $PSScriptRoot
$build = if ($BuildDir) { [IO.Path]::GetFullPath($BuildDir) } else { Join-Path $backend 'build' }
$temporary = Join-Path ([IO.Path]::GetTempPath()) ('vp-study-' + [Guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory $temporary
$database = Join-Path $temporary 'study.db'
$probe = Join-Path $temporary 'db_probe.exe'
$script:server = $null
$script:checks = 0

function Check($Condition, [string]$Message) {
    if (-not $Condition) { throw "FAIL: $Message" }
    $script:checks++
    Write-Host "PASS: $Message"
}

function Sql([string]$Statement) {
    if ($script:server.HasExited) { throw 'Study server exited before SQL probe' }
    $result = & $probe $database $Statement
    if ($LASTEXITCODE -ne 0) { throw 'SQLite Study test probe failed' }
    return $result
}

function Call([string]$Method, [string]$Path, $Body = $null, [string]$Cookie = '') {
    $request = [Net.HttpWebRequest]::Create("http://127.0.0.1:$script:port$Path")
    $request.Method = $Method
    $request.Timeout = 5000
    $request.ReadWriteTimeout = 5000
    $request.Proxy = $null
    if ($Cookie) { $request.Headers['Cookie'] = $Cookie }
    if ($null -ne $Body) {
        $json = if ($Body -is [string]) { $Body } else { ConvertTo-Json -InputObject $Body -Compress -Depth 20 }
        $bytes = [Text.Encoding]::UTF8.GetBytes($json)
        $request.ContentType = 'application/json'
        $request.ContentLength = $bytes.Length
        $stream = $request.GetRequestStream()
        $stream.Write($bytes,0,$bytes.Length)
        $stream.Dispose()
    }
    try { $reply = $request.GetResponse() }
    catch [Net.WebException] {
        if (-not $_.Exception.Response) { throw }
        $reply = $_.Exception.Response
    }
    try {
        $reader = New-Object IO.StreamReader($reply.GetResponseStream())
        $text = $reader.ReadToEnd()
        $content = if ($reply.ContentType -like 'application/json*' -and $text) { ConvertFrom-Json -InputObject $text } else { $text }
        return [pscustomobject]@{ Status = [int]$reply.StatusCode; Body = $content; Headers = $reply.Headers }
    } finally { $reply.Dispose() }
}

function Start-TestServer {
    $listener = New-Object Net.Sockets.TcpListener([Net.IPAddress]::Loopback,0)
    $listener.Start()
    $script:port = $listener.LocalEndpoint.Port
    $listener.Stop()
    $script:server = Start-Process -FilePath (Join-Path $build 'server.exe') -WorkingDirectory (Join-Path $backend 'build') `
        -ArgumentList @($script:port,('"' + $database + '"')) -PassThru `
        -RedirectStandardOutput (Join-Path $temporary 'stdout.log') -RedirectStandardError (Join-Path $temporary 'stderr.log')
    $null = $script:server.Handle
    $deadline = [DateTime]::UtcNow.AddSeconds(10)
    do {
        if ($script:server.HasExited) { throw ('Study server exited: ' + (Get-Content (Join-Path $temporary 'stderr.log') -Raw)) }
        try { if ((Call GET '/api/shop').Status -eq 200) { return } } catch [Net.WebException] {}
    } while ([DateTime]::UtcNow -lt $deadline)
    throw 'Study server startup timed out'
}

function Stop-TestServer {
    if ($script:server -and -not $script:server.HasExited) {
        Stop-Process -Id $script:server.Id -Force
        $script:server.WaitForExit()
    }
    $script:server = $null
}

try {
    if (-not $SkipBuild) {
        & (Join-Path $backend 'build.ps1') -BuildDir $build
        if ($LASTEXITCODE -ne 0) { throw 'Study API build failed' }
    }
    & g++ '-std=c++17' '-O2' '-I' (Join-Path $backend 'third_party/sqlite') `
        (Join-Path $PSScriptRoot 'db_probe.cpp') (Join-Path $build 'sqlite3.o') '-o' $probe
    if ($LASTEXITCODE -ne 0) { throw 'Study SQLite probe compilation failed' }
    & $probe $database "CREATE TABLE study_state (user_id INTEGER PRIMARY KEY REFERENCES users(id) ON DELETE CASCADE,total_points INTEGER NOT NULL DEFAULT 0 CHECK(total_points>=0),happiness_remainder_points INTEGER NOT NULL DEFAULT 0 CHECK(happiness_remainder_points BETWEEN 0 AND 49),elapsed_remainder_seconds INTEGER NOT NULL DEFAULT 0 CHECK(elapsed_remainder_seconds BETWEEN 0 AND 104),is_active INTEGER NOT NULL DEFAULT 0 CHECK(is_active IN (0,1)),last_activity_at INTEGER);"
    if ($LASTEXITCODE -ne 0) { throw 'Could not create legacy Study state for migration test' }

    Start-TestServer
    Check ((Call GET '/api/study/state').Status -eq 401) 'Study API requires authentication'
    $username = 'study_' + [Guid]::NewGuid().ToString('N').Substring(0,8)
    $email = "$username@test.local"
    $registration = Call POST '/api/auth/register' @{username=$username;email=$email;password='study-secret-123'}
    Check ($registration.Status -eq 201) 'Study test account registered'
    $login = Call POST '/api/auth/login' @{email=$email;password='study-secret-123'}
    $cookie = ($login.Headers['Set-Cookie'] -split ';')[0]
    Check ($login.Status -eq 200 -and $cookie.StartsWith('vp_session=')) 'Study test login creates session cookie'
    $userId = [int]$registration.Body.user.id

    $initial = (Call GET '/api/study/state' $null $cookie).Body
    Check ($initial.points -eq 0 -and $initial.coins -eq 100 -and $initial.completedStages.Count -eq 0) 'Study account begins with persistent zero progress'
    Check ((Call POST '/api/study/complete' @{level='A1';tier='medium';correctAnswers=4} $cookie).Status -eq 409) 'Locked tier cannot be completed early'
    $failed = Call POST '/api/study/complete' @{level='A1';tier='easy';correctAnswers=3} $cookie
    Check ($failed.Status -eq 200 -and -not $failed.Body.passed -and $failed.Body.awardedPoints -eq 0) 'Three correct answers do not award provisional points'
    $easy = Call POST '/api/study/complete' @{level='A1';tier='easy';correctAnswers=4} $cookie
    Check ($easy.Body.passed -and $easy.Body.awardedPoints -eq 85 -and $easy.Body.awardedCoins -eq 0) 'Easy tier awards 15 per correct answer plus 25 points'
    Check ($easy.Body.state.pet.level -eq 1 -and $easy.Body.state.pet.PSObject.Properties.Name -notcontains 'experience') 'Study does not expose EXP or level pets through it'
    Check ($easy.Body.state.pet.happiness -eq 1) 'Eighty-five Study points convert to one happiness point'
    $medium = Call POST '/api/study/complete' @{level='A1';tier='medium';correctAnswers=4} $cookie
    Check ($medium.Body.passed -and $medium.Body.awardedPoints -eq 97) 'Medium tier awards 18 per correct answer plus 25 points'
    $hard = Call POST '/api/study/complete' @{level='A1';tier='hard';correctAnswers=4} $cookie
    Check ($hard.Body.passed -and $hard.Body.awardedPoints -eq 130 -and $hard.Body.awardedCoins -eq 20) 'Hard tier awards 20 per correct answer, 50 bonus points and 20 coins'
    $review = Call POST '/api/study/complete' @{level='A1';tier='hard';correctAnswers=5} $cookie
    Check ($review.Body.review -and $review.Body.awardedPoints -eq 0 -and $review.Body.awardedCoins -eq 0) 'Completed stage is review-only and cannot pay twice'
    $nextLevel = Call POST '/api/study/complete' @{level='A2';tier='easy';correctAnswers=4} $cookie
    Check ($nextLevel.Body.passed -and $nextLevel.Body.awardedPoints -eq 85) 'Completing the hard tier unlocks the next level'
    $state = $nextLevel.Body.state
    Check ($state.points -eq 397 -and $state.coins -eq 120) 'Study points and coins update the SQLite account'
    Check ($state.pet.happiness -eq 7 -and $state.pet.level -eq 1) 'Study points convert to happiness using the 50-point remainder'
    Check ((Sql "SELECT happiness_remainder_points FROM study_state WHERE user_id=$userId;") -eq '47') 'Unconverted Study points remain available for the next stage'
    $scaledStudy = Call POST '/api/study/complete' @{level='A2';tier='medium';correctAnswers=4} $cookie
    Check ($scaledStudy.Body.state.pet.level -eq 1 -and $scaledStudy.Body.state.pet.happiness -eq 9) 'Study happiness remains based on points rather than pet level'
    Check ((Sql "SELECT happiness_remainder_points FROM study_state WHERE user_id=$userId;") -eq '44') 'Study point remainder is carried across completed stages'
    $null = Sql "UPDATE pets SET level=2,happiness=0 WHERE user_id=$userId; UPDATE study_state SET happiness_remainder_points=0 WHERE user_id=$userId;"
    $scaledAtLevelTwo = Call POST '/api/study/complete' @{level='A2';tier='hard';correctAnswers=4} $cookie
    Check ($scaledAtLevelTwo.Body.awardedPoints -eq 130 -and $scaledAtLevelTwo.Body.awardedCoins -eq 20) 'Study points and coins remain unchanged at higher pet levels'
    Check ($scaledAtLevelTwo.Body.state.pet.level -eq 2 -and $scaledAtLevelTwo.Body.state.pet.happiness -eq 1) 'Level 2 Study requires 75 points per happiness percent'
    Check ((Sql "SELECT happiness_remainder_points FROM study_state WHERE user_id=$userId;") -eq '55') 'Level 2 Study carries points toward the next happiness percent'
    $purchase = Call POST '/api/shop/purchases' @{itemId=1;quantity=1} $cookie
    Check ($purchase.Status -eq 200 -and $purchase.Body.state.user.coins -eq 135) 'Study coins can buy shop items'
    $null = Sql "UPDATE pets SET hunger=100 WHERE user_id=$userId;"
    $fullFeed = Call POST '/api/pet/feed' @{itemId=1} $cookie
    Check ($fullFeed.Status -eq 422 -and $fullFeed.Body.code -eq 'PET_FULL') 'Pet at 100 hunger cannot eat'
    Check ((Sql "SELECT quantity FROM inventory WHERE user_id=$userId AND item_id=1;") -eq '1') 'Rejected feeding while full keeps food in inventory'
    $null = Sql "UPDATE pets SET hunger=99 WHERE user_id=$userId;"
    $lastBite = Call POST '/api/pet/feed' @{itemId=1} $cookie
    Check ($lastBite.Status -eq 200 -and $lastBite.Body.pet.hunger -eq 100) 'Pet below full can eat until hunger reaches 100'
    Check ((Sql "SELECT count(*) FROM inventory WHERE user_id=$userId AND item_id=1;") -eq '0') 'Successful feeding consumes one food'

    $null = Sql "UPDATE pets SET hunger=30,energy=30 WHERE user_id=$userId;"
    $null = Call POST '/api/study/time' @{action='start'} $cookie
    $null = Sql "UPDATE study_state SET is_active=1,last_activity_at=CAST(strftime('%s','now') AS INTEGER)-30,elapsed_remainder_seconds=75 WHERE user_id=$userId;"
    $tick = Call POST '/api/study/time' @{action='heartbeat'} $cookie
    Check ($tick.Body.pet.hunger -eq 29 -and $tick.Body.pet.energy -eq 29) '105 visible Study seconds reduce hunger and energy by one'
    $null = Call POST '/api/study/time' @{action='pause'} $cookie
    Check ((Sql "SELECT is_active FROM study_state WHERE user_id=$userId;") -eq '0') 'Study timer pauses while the page is hidden'

    $null = Sql "UPDATE pets SET hunger=10,energy=10 WHERE user_id=$userId;"
    $atThreshold = Call POST '/api/study/time' @{action='start'} $cookie
    Check (-not $atThreshold.Body.blocked) 'Pet at exactly 10 percent may continue Study'
    $null = Call POST '/api/study/time' @{action='pause'} $cookie
    $null = Sql "UPDATE pets SET hunger=9,energy=30 WHERE user_id=$userId;"
    $hungerBlocked = Call POST '/api/study/time' @{action='start'} $cookie
    Check ($hungerBlocked.Body.blocked -and $hungerBlocked.Body.pet.hunger -eq 9) 'Study is blocked below 10 percent hunger'
    $blockedReward = Call POST '/api/study/complete' @{level='A1';tier='easy';correctAnswers=5} $cookie
    Check ($blockedReward.Status -eq 409 -and $blockedReward.Body.code -eq 'PET_NEEDS_CARE') 'Server refuses Study rewards while pet needs care'
    $null = Sql "UPDATE pets SET hunger=30,energy=9 WHERE user_id=$userId;"
    $energyBlocked = Call POST '/api/study/time' @{action='start'} $cookie
    Check ($energyBlocked.Body.blocked -and $energyBlocked.Body.pet.energy -eq 9) 'Study is blocked below 10 percent energy'

    Stop-TestServer
    Start-TestServer
    $persisted = (Call GET '/api/study/state' $null $cookie).Body
    Check ($persisted.points -eq 624 -and $persisted.coins -eq 135 -and $persisted.completedStages.Count -eq 6) 'Study points, coins and completed tiers persist after restart'
    Stop-TestServer
    Write-Host "Completed $script:checks Study integration checks."
} finally {
    Stop-TestServer
    if (Test-Path -LiteralPath $temporary) {
        Remove-Item -LiteralPath $temporary -Recurse -Force
    }
}
