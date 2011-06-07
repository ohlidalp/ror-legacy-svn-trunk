// created by thomas fischer (thomas{AT}thomasfischer{DOT}biz) as tool for rigsofrods.com on 9th of June 2011

#include <Windows.h>
#include <stdio.h>

int main(int argc, TCHAR **argv)
{
	if(argc != 3)
	{
		printf("usage: %s <executable> <version output text file>\n", argv[0]);
		printf("       %s MyProgram.exe myprogram_version.txt\n", argv[0]);
		return 1;
	}
	
	
	// http://stackoverflow.com/questions/940707/how-do-i-programatically-get-the-version-of-a-dll-or-exe

	TCHAR *pszFilePath = argv[1];
	DWORD               dwSize              = 0;
	BYTE                *pbVersionInfo      = NULL;
	VS_FIXEDFILEINFO    *pFileInfo          = NULL;
	UINT                puLenFileInfo       = 0;

	// get the version info for the file requested
	dwSize = GetFileVersionInfoSize( pszFilePath, NULL );
	if ( dwSize == 0 )
	{
		printf( "Error in GetFileVersionInfo: %d\n", GetLastError() );
		return 1;
	}

	pbVersionInfo = new BYTE[ dwSize ];

	if ( !GetFileVersionInfo( pszFilePath, 0, dwSize, pbVersionInfo ) )
	{
		printf( "Error in GetFileVersionInfo: %d\n", GetLastError() );
		delete[] pbVersionInfo;
		return 1;
	}

	if ( !VerQueryValue( pbVersionInfo, TEXT("\\"), (LPVOID*) &pFileInfo, &puLenFileInfo ) )
	{
		printf( "Error in VerQueryValue: %d\n", GetLastError() );
		delete[] pbVersionInfo;
		return 1;
	}

	if (pFileInfo->dwSignature == 0xfeef04bd)
	{
		// "The format is as follows: [MAJOR VERSION].[MINOR VERSION].[BUILD NUMBER].[REVISION NUMBER]"
		int major = HIWORD(pFileInfo->dwFileVersionMS);
		int minor = LOWORD(pFileInfo->dwFileVersionMS);
		int build = HIWORD(pFileInfo->dwFileVersionLS);
		int rev   = LOWORD(pFileInfo->dwFileVersionLS);
		FILE *f = fopen(argv[2], "w");
		if(!f)
		{
			printf("unable to open output file\n");
			return 1;
		}
		//fprintf(f, "%d.%d.%d.%d", major, minor, build, rev);
		fprintf(f, "%d.%d.%d", major, minor, build);
		fclose(f);
		printf("done, version written\n");
		return 0;
	}
	printf("error - version not written\n");
	return 1;
}