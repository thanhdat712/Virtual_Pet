param([switch]$SkipBuild, [string]$BuildDir = '')
$ErrorActionPreference = 'Stop'
$backend = Split-Path -Parent $PSScriptRoot
$build = if ($BuildDir) { [IO.Path]::GetFullPath($BuildDir) } else { Join-Path $backend 'build' }
$temporary = Join-Path ([IO.Path]::GetTempPath()) ('vp-practice-' + [Guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory $temporary
$database = Join-Path $temporary 'practice.db'
$probe = Join-Path $temporary 'db_probe.exe'
$script:server = $null
$script:checks = 0

function Check($Condition, [string]$Message) {
    if (-not $Condition) { throw "FAIL: $Message" }
    $script:checks++
    Write-Host "PASS: $Message"
}

function Sql([string]$Statement) {
    if ($script:server.HasExited) { throw 'Practice server exited before SQL probe' }
    $result = & $probe $database $Statement
    if ($LASTEXITCODE -ne 0) { throw 'SQLite Practice test probe failed' }
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
        if ($script:server.HasExited) { throw ('Practice server exited: ' + (Get-Content (Join-Path $temporary 'stderr.log') -Raw)) }
        try { if ((Call GET '/api/shop').Status -eq 200) { return } } catch [Net.WebException] {}
    } while ([DateTime]::UtcNow -lt $deadline)
    throw 'Practice server startup timed out'
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
        if ($LASTEXITCODE -ne 0) { throw 'Practice API build failed' }
    }
    & g++ '-std=c++17' '-O2' '-I' (Join-Path $backend 'third_party/sqlite') `
        (Join-Path $PSScriptRoot 'db_probe.cpp') (Join-Path $build 'sqlite3.o') '-o' $probe
    if ($LASTEXITCODE -ne 0) { throw 'Practice SQLite probe compilation failed' }
    $schema = Get-Content -LiteralPath (Join-Path $backend '..\database\schema.sql') -Raw
    & $probe $database $schema
    if ($LASTEXITCODE -ne 0) { throw 'Could not create the Practice migration test schema' }
    & $probe $database "DROP TABLE practice_state; CREATE TABLE practice_state (user_id INTEGER PRIMARY KEY REFERENCES users(id) ON DELETE CASCADE,total_points INTEGER NOT NULL DEFAULT 0 CHECK(total_points>=0),happiness_remainder_points INTEGER NOT NULL DEFAULT 0 CHECK(happiness_remainder_points BETWEEN 0 AND 19),best_score INTEGER NOT NULL DEFAULT 0 CHECK(best_score>=0),elapsed_remainder_seconds INTEGER NOT NULL DEFAULT 0 CHECK(elapsed_remainder_seconds BETWEEN 0 AND 104),is_active INTEGER NOT NULL DEFAULT 0 CHECK(is_active IN (0,1)),last_activity_at INTEGER,active_game_id TEXT NOT NULL DEFAULT ''); INSERT INTO users(username,email,password_salt,password_hash,email_verified) VALUES('legacy_practice','legacy_practice@test.local','legacy','legacy',1); INSERT INTO pets(user_id,level) SELECT id,1 FROM users WHERE username='legacy_practice'; INSERT INTO practice_state(user_id,total_points,happiness_remainder_points,best_score) SELECT id,45,15,12 FROM users WHERE username='legacy_practice';"
    if ($LASTEXITCODE -ne 0) { throw 'Could not create legacy Practice state for migration test' }

    Start-TestServer
    $practiceColumns = Sql 'PRAGMA table_info(practice_state);'
    Check (($practiceColumns -match '\|happiness_half_percent_remainder\|').Count -eq 1 -and
        ($practiceColumns -match '\|happiness_remainder_points\|').Count -eq 0) 'Practice migration removes the obsolete points column'
    Check ((Sql "SELECT total_points||'|'||happiness_half_percent_remainder||'|'||best_score FROM practice_state WHERE user_id=(SELECT id FROM users WHERE username='legacy_practice');") -eq '45|1|12') 'Practice migration preserves the account record and converts existing half-percent progress'
    Check ((Call GET '/api/practice/state').Status -eq 401) 'Practice API requires authentication'
    $username = 'practice_' + [Guid]::NewGuid().ToString('N').Substring(0,8)
    $email = "$username@test.local"
    $registration = Call POST '/api/auth/register' @{username=$username;email=$email;password='practice-secret-123'}
    Check ($registration.Status -eq 201) 'Practice test account registered'
    $login = Call POST '/api/auth/login' @{email=$email;password='practice-secret-123'}
    $cookie = ($login.Headers['Set-Cookie'] -split ';')[0]
    Check ($login.Status -eq 200 -and $cookie.StartsWith('vp_session=')) 'Practice test login creates session cookie'
    $userId = [int]$registration.Body.user.id

    $initial = (Call GET '/api/practice/state' $null $cookie).Body
    Check ($initial.bestScore -eq 0 -and $initial.points -eq 0 -and $initial.coins -eq 100) 'Practice account starts with a persistent empty record'
    Check ($initial.happinessHalfPercentRemainder -eq 0) 'Practice starts without a pending half-percent happiness reward'

    $null = Sql "UPDATE pets SET hunger=30,energy=30 WHERE user_id=$userId;"
    $null = Call POST '/api/practice/time' @{action='start'} $cookie
    $null = Sql "UPDATE practice_state SET is_active=1,last_activity_at=CAST(strftime('%s','now') AS INTEGER)-30,elapsed_remainder_seconds=75 WHERE user_id=$userId;"
    $tick = Call POST '/api/practice/time' @{action='heartbeat'} $cookie
    Check ($tick.Body.pet.hunger -eq 29 -and $tick.Body.pet.energy -eq 29) '105 seconds on the Practice page reduce hunger and energy by one'

    $firstGame = [Guid]::NewGuid().ToString()
    $null = Call POST '/api/practice/game/start' @{gameId=$firstGame} $cookie
    Check ((Call GET '/api/practice/state' $null $cookie).Body.bestScore -eq 0) 'A provisional record is not visible as the account best before game over'
    $first = Call POST '/api/practice/finish' @{gameId=$firstGame;eaten=4} $cookie
    Check ($first.Body.awardedPoints -eq 0 -and $first.Body.awardedCoins -eq 0 -and $first.Body.state.points -eq 0 -and $first.Body.state.bestScore -eq 4) 'Four foods do not award score points or coins'
    Check ($first.Body.state.pet.happiness -eq 0 -and $first.Body.state.happinessHalfPercentRemainder -eq 0) 'Four foods do not increase happiness or carry food progress into another game'

    $shortGame = [Guid]::NewGuid().ToString()
    $null = Call POST '/api/practice/game/start' @{gameId=$shortGame} $cookie
    $short = Call POST '/api/practice/finish' @{gameId=$shortGame;eaten=1} $cookie
    Check ($short.Body.state.pet.happiness -eq 0 -and $short.Body.state.happinessHalfPercentRemainder -eq 0) 'One food in a later game does not complete the previous game five-food threshold'

    $secondGame = [Guid]::NewGuid().ToString()
    $null = Call POST '/api/practice/game/start' @{gameId=$secondGame} $cookie
    Check ((Call GET '/api/practice/state' $null $cookie).Body.bestScore -eq 4) 'The previous account record remains official while a new game is active'
    Check ((Call GET '/api/game/state' $null $cookie).Body.user.coins -eq 100) 'Record bonus coins remain pending until game over'
    $second = Call POST '/api/practice/finish' @{gameId=$secondGame;eaten=8} $cookie
    Check ($second.Body.awardedPoints -eq 15 -and $second.Body.awardedCoins -eq 15 -and $second.Body.state.coins -eq 115 -and $second.Body.state.bestScore -eq 8) 'Eight foods at level 1 award one 15-point score batch and the record bonus'
    Check ($second.Body.state.pet.happiness -eq 0 -and $second.Body.state.happinessHalfPercentRemainder -eq 1) 'Eight foods at level 1 earn one half-percent and discard the three-food remainder'

    $duplicate = Call POST '/api/practice/finish' @{gameId=$secondGame;eaten=8} $cookie
    Check ($duplicate.Body.duplicate -and (Call GET '/api/game/state' $null $cookie).Body.user.coins -eq 115) 'Repeated game-over requests cannot pay twice'

    $null = Sql "UPDATE pets SET level=2,happiness=0 WHERE user_id=$userId; UPDATE practice_state SET happiness_half_percent_remainder=0 WHERE user_id=$userId;"
    $levelTwoShortOne = [Guid]::NewGuid().ToString()
    $null = Call POST '/api/practice/game/start' @{gameId=$levelTwoShortOne} $cookie
    $levelTwoFour = Call POST '/api/practice/finish' @{gameId=$levelTwoShortOne;eaten=4} $cookie
    Check ($levelTwoFour.Body.state.pet.happiness -eq 0 -and $levelTwoFour.Body.state.happinessHalfPercentRemainder -eq 0) 'Four foods do not meet the level 2 eight-food happiness threshold'

    $levelTwoShortTwo = [Guid]::NewGuid().ToString()
    $null = Call POST '/api/practice/game/start' @{gameId=$levelTwoShortTwo} $cookie
    $levelTwoAnotherFour = Call POST '/api/practice/finish' @{gameId=$levelTwoShortTwo;eaten=4} $cookie
    Check ($levelTwoAnotherFour.Body.state.pet.happiness -eq 0 -and $levelTwoAnotherFour.Body.state.happinessHalfPercentRemainder -eq 0) 'Level 2 food counts do not accumulate across separate games'

    $levelTwoFull = [Guid]::NewGuid().ToString()
    $null = Call POST '/api/practice/game/start' @{gameId=$levelTwoFull} $cookie
    $levelTwoEight = Call POST '/api/practice/finish' @{gameId=$levelTwoFull;eaten=8} $cookie
    Check ($levelTwoEight.Body.awardedPoints -eq 15 -and $levelTwoEight.Body.state.pet.happiness -eq 0 -and $levelTwoEight.Body.state.happinessHalfPercentRemainder -eq 1) 'Eight foods at level 2 earn one half-percent'

    $abandonedGame = [Guid]::NewGuid().ToString()
    $null = Call POST '/api/practice/game/start' @{gameId=$abandonedGame} $cookie
    $null = Call POST '/api/practice/game/abandon' @{gameId=$abandonedGame} $cookie
    Check ((Call GET '/api/practice/state' $null $cookie).Body.bestScore -eq 8) 'Abandoning a provisional record preserves the previous official best'
    Check ((Call POST '/api/practice/finish' @{gameId=$abandonedGame;eaten=15} $cookie).Status -eq 409) 'An explicitly abandoned game cannot later commit its provisional record'

    $restartedGame = [Guid]::NewGuid().ToString()
    $null = Call POST '/api/practice/game/start' @{gameId=$restartedGame} $cookie
    $nextGame = [Guid]::NewGuid().ToString()
    $null = Call POST '/api/practice/game/start' @{gameId=$nextGame} $cookie
    Check ((Call POST '/api/practice/finish' @{gameId=$restartedGame;eaten=15} $cookie).Status -eq 409) 'Starting a new game invalidates the previous unfinished game provisional record'
    $abandoned = Call POST '/api/practice/finish' @{gameId=$nextGame;eaten=4} $cookie
    Check ($abandoned.Body.awardedPoints -eq 0 -and $abandoned.Body.awardedCoins -eq 0 -and $abandoned.Body.state.bestScore -eq 8) 'Restart abandons unfinished rewards and keeps the previous account record'

    $purchase = Call POST '/api/shop/purchases' @{itemId=1;quantity=1} $cookie
    Check ($purchase.Status -eq 200 -and $purchase.Body.state.user.coins -eq 110) 'Practice-earned coins can purchase a Shop item'

    Stop-TestServer
    Start-TestServer
    $persisted = (Call GET '/api/practice/state' $null $cookie).Body
    Check ($persisted.points -eq 30 -and $persisted.bestScore -eq 8 -and $persisted.coins -eq 110 -and $persisted.happinessHalfPercentRemainder -eq 1) 'Practice points, committed record, account currency, and earned half-percent persist after restart'
    Stop-TestServer
    Write-Host "Completed $script:checks Practice integration checks."
} finally {
    Stop-TestServer
    if (Test-Path -LiteralPath $temporary) {
        Remove-Item -LiteralPath $temporary -Recurse -Force
    }
}
