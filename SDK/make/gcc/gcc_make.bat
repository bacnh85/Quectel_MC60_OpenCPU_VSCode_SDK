@ECHO OFF
IF /i "%1" == "new" (
	@copy /y libs\app_image_bin.cfg build\gcc\app_image_bin.cfg
	make\make.exe -f make/gcc/gcc_makefile  2> build/gcc/build.log
) ELSE (
	IF /i "%1" == "clean" (
		make\make.exe %1 -f make/gcc/gcc_makefile
		IF EXIST build\gcc\build.log (
			@del /f build\gcc\build.log
		)
		IF EXIST build\gcc\app_image_bin.cfg (
			@del /f build\gcc\app_image_bin.cfg
		)
	) ELSE (
		IF /i "%1" == "help" (
			make\make.exe %1 -f make/gcc/gcc_makefile
		) ELSE (
			ECHO Incorect input argument.
		)
	)
)