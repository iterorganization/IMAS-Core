@ECHO OFF

GOTO MAIN

:USAGE
ECHO Script used to copy MDSplus model files in new user location,
ECHO and set the relative env variable to use them.
ECHO.
ECHO Usage: newmodel ^<path^> ^<tree_name^>
ECHO.
ECHO   ^<path^>:      Full path where to copy model files
ECHO   ^<tree_name^>: name of the tree, also use in env var
EXIT /B 1

:MAIN
SET "MISSING="
IF "%1"=="" SET MISSING=1
IF "%2"=="" SET MISSING=1
IF DEFINED MISSING (
	CALL :USAGE
) ELSE (
	IF EXIST %1 (
		SET NAME=%2_model
		COPY /Y ids_model.characteristics %1\%NAME%.characteristics
		COPY /Y ids_model.datafile %1\%NAME%.datafile
		COPY /Y ids_model.tree %1\%NAME%.tree
		
		SET VAR=%2_path
		SET %VAR%=%1\
		
		ECHO %VAR% exported, use it in your MDSplus backend
	) ELSE (
		ECHO %1 is not a directory!
	)
)
