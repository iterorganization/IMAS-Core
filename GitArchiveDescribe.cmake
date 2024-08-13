# source GIT_ARCHIVE_DESCRIBE from .git_archival.txt
# git-archive will only expand a single %(describe) placeholder per archive
# https://git-scm.com/docs/gitattributes#_export_subst
file(STRINGS ".git_archival.txt" GIT_ARCHIVE_DESCRIBE REGEX "^describe-name: *")
string(REPLACE "describe-name: " "" GIT_ARCHIVE_DESCRIBE ${GIT_ARCHIVE_DESCRIBE})

