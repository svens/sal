param
(
  [Parameter(Mandatory=$true)]
  [string]$Config,

  [string]$Output = "test_details.xml"
)

$env:GTEST_OUTPUT = "xml:$Output";
$exe = "runas";
$arguments = @( "/trustlevel:0x20000", """ctest --config $Config""" );
Start-Process $exe $arguments -Wait -NoNewWindow;

[xml]$test_details = Get-Content "$Output";
if ($test_details.testsuites.failures -ne 0)
{
  exit 1;
}

exit 0;
