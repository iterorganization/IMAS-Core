# git-archive will only expand a single %(describe) placeholder per archive, so putting
# this in a separate file
# https://git-scm.com/docs/gitattributes#_export_subst
set( GIT_ARCHIVE_DESCRIBE [[$Format:%(describe)$]] )
