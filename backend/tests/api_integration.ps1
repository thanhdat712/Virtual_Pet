param([switch]$SkipBuild, [string]$BuildDir = '')
$ErrorActionPreference = 'Stop'
$backend = Split-Path -Parent $PSScriptRoot
$build = if ($BuildDir) { [IO.Path]::GetFullPath($BuildDir) } else { Join-Path $backend 'build' }
$temporary = Join-Path ([IO.Path]::GetTempPath()) ('vp-api-' + [Guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory $temporary
$database = Join-Path $temporary 'test.db'
$probe = Join-Path $temporary 'db_probe.exe'
$script:server = $null
$script:checks = 0

function Check($Condition, [string]$Message) {
    if (-not $Condition) { throw "FAIL: $Message" }
    $script:checks++
    Write-Host "PASS: $Message"
}

function Sql([string]$Statement) {
    if ($script:server.HasExited) { throw 'Server exited before SQL probe' }
    $result = & $probe $database $Statement
    if ($LASTEXITCODE -ne 0) { throw 'SQLite test probe failed' }
    if ($script:server.HasExited) { throw 'Server exited during SQL probe' }
    return $result
}

function Call([string]$Method, [string]$Path, $Body = $null, [string]$Token = '', [string]$Origin = '', [string]$Cookie = '') {
    $request = [Net.HttpWebRequest]::Create("http://127.0.0.1:$script:port$Path")
    $request.Method = $Method
    $request.Timeout = 5000
    $request.ReadWriteTimeout = 5000
    $request.Proxy = $null
    if ($Token) { $request.Headers['Authorization'] = "Bearer $Token" }
    if ($Origin) { $request.Headers['Origin'] = $Origin }
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

function RegisterAndLogin([string]$Email, [string]$Password) {
    $registration = Call POST '/api/register' @{email=$Email;password=$Password}
    if ($registration.Status -ne 201 -or -not $registration.Body.verificationCode) {
        throw "Could not register test account $Email"
    }
    $verification = Call POST '/api/auth/email-verification/confirm' @{
        email=$Email;verificationCode=$registration.Body.verificationCode
    }
    if ($verification.Status -ne 200) { throw "Could not verify test account $Email" }
    $login = Call POST '/api/login' @{email=$Email;password=$Password}
    if ($login.Status -ne 200 -or -not $login.Body.token) { throw "Could not log in test account $Email" }
    return $login.Body
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
        if ($script:server.HasExited) { throw ('Server exited: ' + (Get-Content (Join-Path $temporary 'stderr.log') -Raw)) }
        try { if ((Call GET '/api/shop').Status -eq 200) { return } } catch [Net.WebException] {}
    } while ([DateTime]::UtcNow -lt $deadline)
    throw 'Server startup timed out'
}

function Stop-TestServer {
    if ($script:server -and -not $script:server.HasExited) {
        Stop-Process -Id $script:server.Id -Force
        $script:server.WaitForExit()
    }
    $script:server = $null
}

function Raw([string]$Request) {
    $client = New-Object Net.Sockets.TcpClient
    try {
        $client.Connect('127.0.0.1',$script:port)
        $stream = $client.GetStream()
        $stream.ReadTimeout = 6000
        $bytes = [Text.Encoding]::ASCII.GetBytes($Request)
        $stream.Write($bytes,0,$bytes.Length)
        $reader = New-Object IO.StreamReader($stream)
        return $reader.ReadToEnd()
    } finally { $client.Dispose() }
}

try {
    if (-not $SkipBuild) { & (Join-Path $backend 'build.ps1') -BuildDir $build }
    & g++ '-std=c++17' '-O2' '-I' (Join-Path $backend 'third_party/sqlite') `
        (Join-Path $PSScriptRoot 'db_probe.cpp') (Join-Path $build 'sqlite3.o') '-o' $probe
    if ($LASTEXITCODE -ne 0) { throw 'Test probe compilation failed' }
    Start-TestServer
    $catalog = (Call GET '/api/shop').Body
    Check ($catalog.Count -eq 13 -and $catalog[0].price -eq 5 -and $catalog[0].icon) '13-item catalog with frontend fields'
    foreach ($item in $catalog | Where-Object { $_.category -eq 'decoration' }) {
        Check ((Call GET ('/' + $item.image)).Status -eq 200) "Decoration image is served for item $($item.id)"
    }
    foreach ($item in $catalog | Where-Object { $_.filter -eq 'headwear' }) {
        Check ($item.petImage -and (Call GET ('/' + $item.petImage)).Status -eq 200) "Cat headwear sprite is served for item $($item.id)"
    }
    Check ((Call GET '/').Status -eq 200) 'Same-origin index served'
    Check ((Call GET '/js/api.js').Status -eq 200) 'Same-origin JavaScript served'
    Check ((Call GET '/%2e%2e/backend/build/token.txt').Status -eq 403) 'Encoded traversal rejected'
    Check ((Raw "GET /../backend/build/token.txt HTTP/1.1`r`nHost: 127.0.0.1:$script:port`r`n`r`n") -match '^HTTP/1.1 403') 'Literal traversal rejected'
    Check ((Call GET '/api/shop' $null '' 'https://evil.example').Status -eq 403) 'Untrusted origin rejected'
    foreach ($origin in @('http://localhost:5500','http://127.0.0.1:5500','null',"http://127.0.0.1:$script:port")) {
        $allowed = Call OPTIONS '/api/shop' $null '' $origin
        Check ($allowed.Status -eq 204 -and $allowed.Headers['Access-Control-Allow-Origin'] -eq $origin) "Allowed origin $origin"
    }
    Check ((Call GET '/api/me').Status -eq 401) 'Anonymous profile rejected'
    Check ((Call POST '/api/shop/purchases' @{itemId=1;quantity=1}).Status -eq 401) 'Anonymous purchase rejected'
    Check ((Call POST '/api/pet/feed' @{itemId=1}).Status -eq 401) 'Anonymous feeding rejected'
    $alice = RegisterAndLogin 'alice@test.local' 'test-secret-123'
    $bob = RegisterAndLogin 'bob@test.local' 'test-secret-123'
    Check ($alice.token.Length -eq 64 -and $alice.user.coins -eq 100) 'Register creates local session and initial balance'
    Check ((Call POST '/api/register' @{email='alice@test.local';password='test-secret-123'}).Status -eq 409) 'Duplicate registration rejected'
    Check ((Call POST '/api/login' @{email='alice@test.local';password='wrong-password'}).Status -eq 401) 'Wrong password rejected'
    $stored = (Sql "SELECT password_salt,password_hash FROM users WHERE id=1;").Split('|')
    Check ($stored[0].Length -eq 32 -and $stored[1].StartsWith('pbkdf2-sha256$600000$')) 'Random salt and PBKDF2 600000 stored'
    $derive = [Security.Cryptography.Rfc2898DeriveBytes]::new('test-secret-123',[Text.Encoding]::UTF8.GetBytes($stored[0]),600000,[Security.Cryptography.HashAlgorithmName]::SHA256)
    try { $expected = ([BitConverter]::ToString($derive.GetBytes(32))).Replace('-','').ToLowerInvariant() } finally { $derive.Dispose() }
    Check ($stored[1] -eq ('pbkdf2-sha256$600000$' + $expected)) 'CNG hash agrees with independent .NET PBKDF2 implementation'
    Check ((Sql "SELECT count(DISTINCT password_salt) FROM users;") -eq '2') 'Each user gets a distinct salt'
    Check ((Sql "SELECT count(*) FROM sessions WHERE expires_at BETWEEN CAST(strftime('%s','now') AS INTEGER)+604700 AND CAST(strftime('%s','now') AS INTEGER)+604800;") -eq '2') 'Sessions expire in seven days'
    Check ((Sql "SELECT count(*) FROM sessions WHERE token='$($alice.token)';") -eq '0') 'Bearer tokens are not stored in plaintext'
    $purchase = Call POST '/api/shop/purchases' @{itemId=1;quantity=2;price=0;userId=2} $alice.token
    Check ($purchase.Status -eq 200 -and $purchase.Body.user.coins -eq 90 -and $purchase.Body.inventory[0].quantity -eq 2) 'Authoritative pricing ignores client price and user ID'
    $purchase = Call POST '/api/shop/purchases' @{itemId=1;quantity=1} $alice.token
    Check ($purchase.Body.user.coins -eq 85 -and $purchase.Body.inventory[0].quantity -eq 3) 'Food stacks'
    $purchase = Call POST '/api/shop/purchases' @{itemId=6;quantity=1} $alice.token
    Check ($purchase.Body.user.coins -eq 70) 'Decoration purchase charges authoritative price'
    Check ((Call POST '/api/shop/purchases' @{itemId=6;quantity=1} $alice.token).Status -eq 409) 'Duplicate decoration rejected'
    foreach ($body in @(@{itemId=6;quantity=2},@{itemId=1;quantity=0},@{itemId=1;quantity=-1},@{itemId=1;quantity=1.5},@{itemId=1;quantity='2'},@{itemId=1;quantity=1001},@{itemId=1;quantity=$true})) {
        Check ((Call POST '/api/shop/purchases' $body $alice.token).Status -eq 400) 'Invalid purchase quantity rejected'
    }
    Check ((Call POST '/api/shop/purchases' @{itemId=999;quantity=1} $alice.token).Status -eq 404) 'Unknown item rejected'
    Check ((Call POST '/api/shop/purchases' @{itemId=3;quantity=100} $alice.token).Status -eq 409) 'Insufficient funds rejected'
    $me = (Call GET '/api/me' $null $alice.token).Body
    Check ($me.user.coins -eq 70 -and $me.inventory.Count -eq 2) 'Rejected purchases leave balance and inventory unchanged'
    Check ((Sql 'SELECT count(*),sum(total) FROM purchase_transactions;') -eq '3|30') 'Exactly successful purchases appear in history'
    $other = (Call GET '/api/me' $null $bob.token).Body
    Check ($other.user.coins -eq 100 -and $other.inventory.Count -eq 0 -and $other.progress.Count -eq 0) 'Users cannot mutate another inventory or balance'
    $aliceBeforeFeed = ConvertTo-Json -InputObject $me -Compress -Depth 20
    $bobBeforeFeed = ConvertTo-Json -InputObject $other -Compress -Depth 20
    foreach ($body in @(@{},@{itemId=0},@{itemId=-1},@{itemId=1.5},@{itemId='1'},@{itemId=$true},@{itemId=$null},'{broken','[]','{"itemId":18446744073709551615}')) {
        Check ((Call POST '/api/pet/feed' $body $alice.token).Status -eq 400) 'Invalid feeding body rejected'
    }
    foreach ($itemId in @(6,7)) {
        Check ((Call POST '/api/pet/feed' @{itemId=$itemId} $alice.token).Status -eq 400) 'Nonfood rejected regardless of ownership'
    }
    Check ((Call POST '/api/pet/feed' @{itemId=999} $alice.token).Status -eq 404) 'Unknown feeding item rejected'
    Check ((Call POST '/api/pet/feed' @{itemId=2} $alice.token).Status -eq 409) 'Unowned food rejected'
    Check ((Call POST '/api/pet/feed' @{itemId=1;userId=$alice.user.id} $bob.token).Status -eq 409) 'Another user cannot consume owned food'
    $feeder = RegisterAndLogin 'feeder@test.local' 'test-secret-123'
    $restores = @(@(30,0),@(20,5),@(40,15),@(10,20),@(5,25))
    for ($index=0; $index -lt 5; $index++) {
        $itemId = $index + 1
        $quantity = if ($itemId -eq 1) { 2 } else { 1 }
        $food = $catalog | Where-Object { $_.id -eq $itemId }
        Check ($food.hungerRestore -eq $restores[$index][0] -and $food.happinessRestore -eq $restores[$index][1]) "Food $itemId catalog matches shop restore values"
        Check ((Call POST '/api/shop/purchases' @{itemId=$itemId;quantity=$quantity} $feeder.token).Status -eq 200) "Buy food $itemId for feeding"
        $null = Sql "UPDATE pets SET hunger=10,happiness=10 WHERE user_id=$($feeder.user.id);"
        if ($itemId -eq 1) {
            $null = Sql "CREATE TRIGGER fail_feed BEFORE UPDATE ON pets WHEN OLD.user_id=$($feeder.user.id) BEGIN SELECT RAISE(ABORT,'forced feeding failure'); END;"
            Check ((Call POST '/api/pet/feed' @{itemId=1} $feeder.token).Status -eq 500) 'Database failure returns feeding error'
            $null = Sql 'DROP TRIGGER fail_feed;'
            Check ((Sql "SELECT quantity FROM inventory WHERE user_id=$($feeder.user.id) AND item_id=1;") -eq '2' -and (Sql "SELECT hunger,happiness FROM pets WHERE user_id=$($feeder.user.id);") -eq '10|10') 'Failed pet update rolls back consumption and stats'
        }
        $fed = Call POST '/api/pet/feed' @{itemId=$itemId;userId=$bob.user.id;quantity=999;hunger=100;happiness=100;hungerRestore=100;happinessRestore=100;coins=99999;energy=100;level=99} $feeder.token
        Check ($fed.Status -eq 200 -and $fed.Body.pet.hunger -eq (10+$restores[$index][0]) -and $fed.Body.pet.happiness -eq (10+$restores[$index][1])) "Food $itemId applies only authoritative restores"
        Check ($fed.Body.user.id -eq $feeder.user.id -and $fed.Body.pet.energy -eq 70 -and $fed.Body.pet.level -eq 1 -and $fed.Body.pet.experience -eq 0) 'Feeding ignores forged user and unrelated stats'
        if ($itemId -eq 1) {
            Check ($fed.Body.inventory.Count -eq 1 -and $fed.Body.inventory[0].quantity -eq 1) 'Feeding decrements stack by exactly one'
            $null = Sql "UPDATE pets SET hunger=95,happiness=99 WHERE user_id=$($feeder.user.id);"
            $fed = Call POST '/api/pet/feed' @{itemId=1} $feeder.token
            Check ($fed.Status -eq 200 -and $fed.Body.pet.hunger -eq 100 -and $fed.Body.pet.happiness -eq 99) 'Fullness clamps to 100 and bread has no happiness restore'
        }
        Check ($fed.Body.inventory.Count -eq 0) "Last food $itemId is deleted rather than stored at zero"
        Check ((Call POST '/api/pet/feed' @{itemId=$itemId} $feeder.token).Status -eq 409) "Depleted food $itemId cannot be consumed again"
    }
    $feederMe = (Call GET '/api/me' $null $feeder.token).Body
    Check ($feederMe.user.coins -eq 48 -and $feederMe.pet.hunger -eq 15 -and $feederMe.pet.happiness -eq 35) 'Feeding never charges coins'
    Check ((ConvertTo-Json -InputObject $fed.Body.user -Compress) -eq (ConvertTo-Json -InputObject $feederMe.user -Compress) -and (ConvertTo-Json -InputObject $fed.Body.pet -Compress) -eq (ConvertTo-Json -InputObject $feederMe.pet -Compress)) 'Feeding response uses profile user and pet formats'
    Check ((Sql "SELECT count(*) FROM inventory WHERE user_id=$($feeder.user.id);") -eq '0') 'Depleted inventory rows are absent in SQLite'
    Check ((ConvertTo-Json -InputObject (Call GET '/api/me' $null $alice.token).Body -Compress -Depth 20) -eq $aliceBeforeFeed) 'Rejected feeding and other users leave Alice unchanged'
    Check ((ConvertTo-Json -InputObject (Call GET '/api/me' $null $bob.token).Body -Compress -Depth 20) -eq $bobBeforeFeed) 'Forged feeding target leaves Bob unchanged'
    Check ((Call POST '/api/shop/purchases' @{itemId=5;quantity=1} $feeder.token).Status -eq 200) 'Buy honey for happiness clamp check'
    $null = Sql "UPDATE pets SET hunger=100,happiness=95 WHERE user_id=$($feeder.user.id);"
    $fed = Call POST '/api/pet/feed' @{itemId=5} $feeder.token
    Check ($fed.Status -eq 200 -and $fed.Body.pet.hunger -eq 100 -and $fed.Body.pet.happiness -eq 20 -and $fed.Body.pet.level -eq 2 -and $fed.Body.inventory.Count -eq 0) 'Happiness overflow levels up and preserves its remainder'
    Check ((Call PUT '/api/pet' @{coins=99999;level=99;energy=100} $alice.token).Status -eq 400) 'Client stat and coin updates rejected'
    $renamed = Call PUT '/api/pet' @{name='API Pet'} $alice.token
    Check ($renamed.Body.pet.name -eq 'API Pet' -and $renamed.Body.pet.level -eq 1 -and $renamed.Body.user.coins -eq 70) 'Name-only pet update works without stat changes'
    $exercises = (Call GET '/api/exercises').Body
    Check ($exercises.Count -eq 5 -and $exercises[1].description.Contains('7 + 5')) 'Five explicit deterministic exercises'
    $answers = @('Hello World!','12','even','9','1 2 3 4 5')
    for ($index=0; $index -lt 5; $index++) {
        $id = $index+1
        $failed = Call POST "/api/exercises/$id/submissions" @{answer='wrong';score=100;status='completed'} $alice.token
        Check ($failed.Body.score -eq 0 -and $failed.Body.status -eq 'in_progress' -and $failed.Body.progress.bestScore -eq 0) "Exercise $id ignores forged score and status"
        $passed = Call POST "/api/exercises/$id/submissions" @{answer=$answers[$index];score=0;status='failed'} $alice.token
        Check ($passed.Body.score -eq 100 -and $passed.Body.status -eq 'completed') "Exercise $id grades correct answer"
        $later = Call POST "/api/exercises/$id/submissions" @{answer='wrong'} $alice.token
        Check ($later.Body.score -eq 0 -and $later.Body.progress.status -eq 'completed' -and $later.Body.progress.bestScore -eq 100 -and $later.Body.progress.attempts -eq 3 -and $later.Body.progress.completedAt -eq $passed.Body.progress.completedAt) "Exercise $id preserves completed progress after failure"
    }
    Check ((Sql "SELECT count(*) FROM submissions WHERE answer='wrong' AND score=0;") -eq '10') 'Actual submitted answers stored'
    Check ((Call POST '/api/exercises/1/submissions' @{score=100} $alice.token).Status -eq 400) 'Score-only submission rejected'
    Check ((Call POST '/api/exercises/1/submissions' @{answer=12} $alice.token).Status -eq 400) 'Non-string answer rejected'
    Check ((Call POST '/api/exercises/99/submissions' @{answer='12'} $alice.token).Status -eq 404) 'Unknown exercise rejected'
    Check ((Call POST '/api/shop/purchases' '{broken' $alice.token).Status -eq 400) 'Malformed JSON rejected without crashing'
    Check ((Call POST '/api/shop/purchases' '[]' $alice.token).Status -eq 400) 'Non-object JSON rejected'
    $nested = '{"extra":' + ('[' * 20) + '0' + (']' * 20) + '}'
    Check ((Call POST '/api/shop/purchases' $nested $alice.token).Status -eq 400) 'Excessive JSON nesting rejected'
    Check ((Call POST '/api/exercises/1/submissions' @{answer=('x' * 8193)} $alice.token).Status -eq 400) 'Oversized exercise answer rejected'
    Check ((Call POST '/api/shop/purchases' '{"itemId":1,"quantity":18446744073709551615}' $alice.token).Status -eq 400) 'Integer overflow purchase rejected'
    Check ((Raw "POST /api/login HTTP/1.1`r`nHost: 127.0.0.1:$script:port`r`nContent-Length: 32769`r`n`r`n") -match '^HTTP/1.1 413') 'Oversized body rejected before body read'
    Check ((Raw "POST /api/login HTTP/1.1`r`nHost: 127.0.0.1:$script:port`r`nContent-Length: 0`r`nContent-Length: 0`r`n`r`n") -match '^HTTP/1.1 400') 'Duplicate lengths rejected'
    Check ((Raw "POST /api/login HTTP/1.1`r`nHost: 127.0.0.1:$script:port`r`nTransfer-Encoding: chunked`r`n`r`n") -match '^HTTP/1.1 400') 'Unsupported transfer encoding rejected'
    Check ((Raw "GET /api/shop HTTP/1.1`r`nHost: evil.example`r`n`r`n") -match '^HTTP/1.1 403') 'Non-local Host rejected'
    Check ((Raw "GET /api/shop HTTP/1.1`r`nHost: 127.0.0.1:$script:port`r`nX-Test: $('x' * 17000)`r`n`r`n") -match '^HTTP/1.1 431') 'Oversized headers rejected'
    Check ((Raw "POST /api/login HTTP/1.1`r`nHost: 127.0.0.1:$script:port`r`nContent-Length: -1`r`n`r`n") -match '^HTTP/1.1 400') 'Negative content length rejected'
    Check ((Raw "POST /api/login HTTP/1.1`r`nHost: 127.0.0.1:$script:port`r`nContent-Length: 20`r`n`r`n{") -match '^HTTP/1.1 400') 'Incomplete body times out rather than blocking indefinitely'
    Check ((Call GET '/api/shop').Status -eq 200) 'Server remains responsive after timed-out request'
    Stop-TestServer
    Start-TestServer
    $persisted = (Call GET '/api/me' $null $alice.token).Body
    Check ($persisted.user.coins -eq 70 -and $persisted.inventory.Count -eq 2 -and $persisted.pet.name -eq 'API Pet' -and $persisted.progress.Count -eq 5) 'Profile inventory progress and session persist after restart'
    $persistedFeed = (Call GET '/api/me' $null $feeder.token).Body
    Check ($persistedFeed.user.coins -eq 45 -and $persistedFeed.pet.hunger -eq 100 -and $persistedFeed.pet.happiness -eq 20 -and $persistedFeed.pet.level -eq 2 -and $persistedFeed.inventory.Count -eq 0) 'Feeding level and stats persist after restart'
    Check ((Call POST '/api/pet/feed' @{itemId=5} $feeder.token).Status -eq 409) 'Restart does not restore consumed food'
    Check ((Call GET '/api/me' $null $bob.token).Body.progress.Count -eq 0) 'Exercise progress remains user-isolated'
    Check ((Call POST '/api/logout' @{} $alice.token).Status -eq 200) 'Logout succeeds'
    Check ((Call GET '/api/me' $null $alice.token).Status -eq 401) 'Revoked session cannot authenticate'
    Stop-TestServer
    Start-TestServer
    Check ((Call GET '/api/me' $null $alice.token).Status -eq 401) 'Logout remains revoked across restart'
    Check ((Call GET '/api/me' $null $bob.token).Status -eq 200) 'Logout does not revoke another user session'
    $login = Call POST '/api/login' @{email='alice@test.local';password='test-secret-123'}
    Check ($login.Status -eq 200) 'PBKDF2 login succeeds'
    $null = Sql "UPDATE sessions SET expires_at=0 WHERE user_id=1;"
    Check ((Call GET '/api/me' $null $login.Body.token).Status -eq 401) 'Expired session rejected'
    $sha = [Security.Cryptography.SHA256]::Create()
    try { $legacy = ([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes('legacy-salttest-secret-123')))).Replace('-','').ToLowerInvariant() } finally { $sha.Dispose() }
    $null = Sql "UPDATE users SET password_salt='legacy-salt',password_hash='$legacy' WHERE id=1;"
    Check ((Call POST '/api/login' @{email='alice@test.local';password='wrong-password'}).Status -eq 401) 'Legacy wrong password rejected'
    Check ((Sql "SELECT password_hash FROM users WHERE id=1;") -eq $legacy) 'Failed legacy login does not migrate hash'
    Check ((Call POST '/api/login' @{email='alice@test.local';password='test-secret-123'}).Status -eq 200) 'Legacy login succeeds'
    Check ((Sql "SELECT password_hash FROM users WHERE id=1;").StartsWith('pbkdf2-sha256$600000$')) 'Successful legacy login migrates hash'
    Stop-TestServer
    Start-TestServer
    for ($attempt=0; $attempt -lt 12; $attempt++) {
        Check ((Call POST '/api/login' @{email='alice@test.local';password='test-secret-123'}).Status -eq 200) 'Successful logins do not consume failure budget'
        Check ((Call POST '/api/login' @{email='alice@test.local'}).Status -eq 400) 'Malformed login does not consume credential failure budget'
    }
    for ($attempt=0; $attempt -lt 10; $attempt++) {
        $email = if ($attempt % 2 -eq 0) { 'alice@test.local' } else { "unknown-$attempt@test.local" }
        Check ((Call POST '/api/login' @{email=$email;password='wrong-password'}).Status -eq 401) 'Credential failures consume shared bounded budget'
        Check ((Call GET '/api/me' $null $bob.token).Status -eq 200) 'Authenticated requests remain available during failed logins'
        if ($attempt -lt 9) {
            Check ((Call POST '/api/login' @{email='bob@test.local';password='test-secret-123'}).Status -eq 200) 'Interleaved successful logins do not erase earlier failures'
        }
    }
    $limited = Call POST '/api/login' @{email='alice@test.local';password='test-secret-123'}
    Check ($limited.Status -eq 429 -and [int]$limited.Headers['Retry-After'] -ge 1 -and [int]$limited.Headers['Retry-After'] -le 30) 'Exhausted failure budget returns 429 with bounded Retry-After'
    Check ((Call POST '/api/login' @{email='another-unknown@test.local';password='wrong-password'}).Status -eq 429) 'Rotating email cannot bypass failure limit'
    Check ((Call GET '/api/shop').Status -eq 200) 'Public requests are not rate limited'
    Check ((Call GET '/api/me' $null $bob.token).Status -eq 200) 'Existing sessions remain usable during login cooldown'
    Check ((Call POST '/api/register' @{email='cooldown@test.local';password='test-secret-123'}).Status -eq 201) 'Registration is not charged to login failure budget'
    Check ((Call POST '/api/login' @{email='alice@test.local';password='wrong-password'}).Status -eq 429) 'Other auth activity does not reset login failures'
    Stop-TestServer
    Start-TestServer
    Check ((Call POST '/api/login' @{email='alice@test.local';password='test-secret-123'}).Status -eq 200) 'Process-local failure budget resets on restart'

    Check (-not (Call GET '/api/auth/session').Body.authenticated) 'Frontend session route reports anonymous state'
    $frontendAccount = Call POST '/api/auth/register' @{username='frontend_01';email='frontend@test.local';password='frontend-secret-123'}
    Check ($frontendAccount.Status -eq 201 -and $frontendAccount.Body.success -and $frontendAccount.Body.user.username -eq 'frontend_01') 'Frontend registration contract works'
    Check ((Call POST '/api/auth/register' @{username='frontend_01';email='other@test.local';password='frontend-secret-123'}).Status -eq 409) 'Frontend duplicate username rejected'
    Check ((Call POST '/api/auth/email-verification/confirm' @{email='frontend@test.local';verificationCode=$frontendAccount.Body.verificationCode}).Status -eq 200) 'Frontend email verification succeeds'
    $frontendLogin = Call POST '/api/auth/login' @{email='frontend@test.local';password='frontend-secret-123'}
    $frontendCookie = ($frontendLogin.Headers['Set-Cookie'] -split ';')[0]
    Check ($frontendLogin.Status -eq 200 -and $frontendLogin.Body.success -and $frontendLogin.Body.user.username -eq 'frontend_01' -and $frontendCookie.StartsWith('vp_session=')) 'Frontend login sets HttpOnly session cookie'
    Check ((Call GET '/api/auth/session' $null '' '' $frontendCookie).Body.authenticated) 'Frontend cookie restores login session'
    $frontendState = (Call GET '/api/game/state' $null '' '' $frontendCookie).Body
    Check ($frontendState.user.username -eq 'frontend_01' -and $frontendState.pet.species -eq 'cat' -and $frontendState.inventory.Count -eq 0) 'Frontend game state loads from database'
    Check ((Call GET '/api/exercises').Body.Count -eq 5) 'Exercise API returns catalog'
    $frontendSubmission = Call POST '/api/exercises/1/submissions' @{answer='Hello World!'} '' '' $frontendCookie
    Check ($frontendSubmission.Status -eq 201 -and $frontendSubmission.Body.score -eq 100) 'Exercise API grades and stores submissions'
    $frontendPurchase = Call POST '/api/shop/purchases' @{itemId=8;quantity=1} '' '' $frontendCookie
    Check ($frontendPurchase.Body.success -and $frontendPurchase.Body.state.user.coins -eq 80) 'Frontend purchase returns action state'
    $frontendPurchase = Call POST '/api/shop/purchases' @{itemId=1;quantity=1} '' '' $frontendCookie
    Check ($frontendPurchase.Body.success -and $frontendPurchase.Body.state.inventory[0].itemId -eq 1) 'Frontend food purchase updates database inventory'
    $frontendFeed = Call POST '/api/pet/feed' @{itemId=1} '' '' $frontendCookie
    Check ($frontendFeed.Body.success -and $frontendFeed.Body.state.pet.hunger -eq 60 -and $frontendFeed.Body.state.inventory.Count -eq 1 -and $frontendFeed.Body.state.inventory[0].itemId -eq 8) 'Frontend feeding consumes food and keeps owned decoration'
    $frontendEquip = Call POST '/api/pet/decorate' @{itemId=8} '' '' $frontendCookie
    Check ($frontendEquip.Body.success -and $frontendEquip.Body.state.equipment.background -eq 8) 'Frontend decoration equipment persists'
    $frontendPurchase = Call POST '/api/shop/purchases' @{itemId=10;quantity=1} '' '' $frontendCookie
    Check ($frontendPurchase.Body.success -and (@($frontendPurchase.Body.state.inventory | Where-Object { $_.itemId -eq 10 }).Count -eq 1)) 'Frontend floor purchase updates database inventory'
    $frontendEquip = Call POST '/api/pet/decorate' @{itemId=10} '' '' $frontendCookie
    Check ($frontendEquip.Body.success -and $frontendEquip.Body.state.equipment.floor -eq 10) 'Frontend floor equipment persists'
    $frontendProfile = Call PUT '/api/pet' @{name='Mochi';species='dog'} '' '' $frontendCookie
    Check ($frontendProfile.Body.state.pet.name -eq 'Mochi' -and $frontendProfile.Body.state.pet.species -eq 'dog') 'Pet name and species are saved in SQLite'
    $frontendSettings = Call PUT '/api/settings' @{soundEnabled=$false} '' '' $frontendCookie
    Check (-not $frontendSettings.Body.settings.soundEnabled) 'Sound preference is saved in SQLite'
    $frontendPlay = Call POST '/api/pet/play' $null '' '' $frontendCookie
    Check ($frontendPlay.Body.state.pet.level -eq 2 -and $frontendPlay.Body.state.pet.energy -eq 55) 'Frontend play action updates game state'
    $frontendSleep = Call POST '/api/pet/sleep' $null '' '' $frontendCookie
    Check ($frontendSleep.Body.state.pet.energy -eq 95) 'Frontend sleep action updates game state'
    $frontendBath = Call POST '/api/pet/bath' $null '' '' $frontendCookie
    Check ($frontendBath.Body.state.pet.happiness -eq 5) 'Frontend bath action updates game state'
    $null = Call POST '/api/pet/unequip' @{slot='background'} '' '' $frontendCookie
    Stop-TestServer
    Start-TestServer
    $frontendPersisted = (Call GET '/api/game/state' $null '' '' $frontendCookie).Body
    Check ($frontendPersisted.pet.name -eq 'Mochi' -and $frontendPersisted.pet.species -eq 'dog' -and $frontendPersisted.pet.level -eq 2 -and $frontendPersisted.settings.soundEnabled -eq $false -and $null -eq $frontendPersisted.equipment.background -and $frontendPersisted.equipment.floor -eq 10) 'Frontend profile, settings, actions and equipment persist across restart'
    Check ((Call POST '/api/auth/logout' @{} '' '' $frontendCookie).Body.success) 'Frontend logout revokes cookie session'
    Check (-not (Call GET '/api/auth/session' $null '' '' $frontendCookie).Body.authenticated) 'Frontend logout prevents session reuse'

    Stop-TestServer
    $legacyDatabase = Join-Path $temporary 'legacy.db'
    $legacySql = "CREATE TABLE users(id INTEGER PRIMARY KEY AUTOINCREMENT,email TEXT NOT NULL UNIQUE,password_salt TEXT NOT NULL,password_hash TEXT NOT NULL,coins INTEGER NOT NULL DEFAULT 100,created_at TEXT NOT NULL DEFAULT (datetime('now')),last_login_at TEXT);"
    $legacySql += "CREATE TABLE pets(user_id INTEGER PRIMARY KEY REFERENCES users(id),name TEXT NOT NULL DEFAULT 'UIA',level INTEGER NOT NULL DEFAULT 1,experience INTEGER NOT NULL DEFAULT 0,happiness INTEGER NOT NULL DEFAULT 85,hunger INTEGER NOT NULL DEFAULT 30,energy INTEGER NOT NULL DEFAULT 70);"
    $legacySql += "INSERT INTO users(id,email,password_salt,password_hash,coins) VALUES(7,'legacy@test.local','legacy-salt','legacy-hash',77);"
    $legacySql += "INSERT INTO pets(user_id,name,level,experience,happiness,hunger,energy) VALUES(7,'Old Pet',3,5,42,55,66);"
    $null = & $probe $legacyDatabase $legacySql
    if ($LASTEXITCODE -ne 0) { throw 'Could not create legacy migration fixture' }
    $database = $legacyDatabase
    Start-TestServer
    Check ((Sql 'SELECT username,coins,sound_enabled FROM users WHERE id=7;') -eq 'player_7|77|1') 'Legacy user data migrates without losing coins'
    Check ((Sql 'SELECT name,species,level,happiness,hunger,energy FROM pets WHERE user_id=7;') -eq 'Old Pet|cat|3|42|55|66') 'Legacy pet data migrates without losing stats'
    Write-Host "API integration: $script:checks checks passed"
} catch {
    if ($script:server) {
        $script:server.Refresh()
        Write-Host "Server exited: $($script:server.HasExited); exit code: $($script:server.ExitCode)"
    }
    Get-Content (Join-Path $temporary 'stderr.log') -ErrorAction SilentlyContinue | Write-Host
    throw
} finally {
    Stop-TestServer
    Remove-Item $temporary -Recurse -Force
}