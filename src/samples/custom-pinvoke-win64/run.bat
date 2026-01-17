
call build.bat

call dotnet build ..\..\tests\managed\CoreTests\CoreTests.csproj

build\bin\Debug\custom-pinvoke.exe -l ..\..\libraries\dotnetframework4.x -l ..\..\tests\managed\CoreTests\bin\Debug -e test.App::CallCustomPInvoke CoreTests

pause