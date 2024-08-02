# Stop when any command fails
$ErrorActionPreference = "Stop"
# Enable debug output
$DebugPreference = "Continue"

$sharepoint_root = "\\sharepoint.iter.org@SSL\departments\POP\CM\IMDesign\Code Documentation\ACCESS-LAYER-doc"
$root_url = "https://sharepoint.iter.org/departments/POP/CM/IMDesign/Code%20Documentation/ACCESS-LAYER-doc"
$version = "##VERSION##"

Net use X: $sharepoint_root

foreach ($hli in $("cpp", "fortran", "java", "matlab", "python")) {
	$deploy_folder = "${sharepoint_root}\${hli}\${version}"
	Write-Debug "Deploying to folder: $deploy_folder"

	New-Item -ItemType Directory -Path $deploy_folder -Force
	Copy-Item -Path ".\${hli}_html\*" -Destination $deploy_folder -Recurse -Force

	# Generate versions.js
	$latest = "IDM"
	$latest_version = [version]"0.0"
	$latest_url = "https://user.iter.org/default.aspx?uid=YSQENW"

	$versions = New-Object System.Collections.Generic.List[string]
	Get-ChildItem -Path "${sharepoint_root}\${hli}" -Directory -Name | Foreach-Object {
		try {
			$as_version = [version]$_
			if ($as_version -gt $latest_version) {
				$latest_version = $as_version
				$latest = $_
                $latest_url = "${root_url}/${hli}/${_}/index.html"
			}
		} catch [System.InvalidCastException] {
			sleep 0.01
			Write-Debug "Could not convert $_ to a [version]. Ignoring for version comparisons"
		}
	}
	# Generate the versions.js file for use by the sphinx-immaterial version
	# switcher. See
	# https://jbms.github.io/sphinx-immaterial/customization.html#version-dropdown
	#
	# Notes:
	# -	The "version" item is used for generating the URL. Since sharepoint
	#   doesn't automatically forward to `index.html`, we add it manually. We
	#   also need to include a pound (#) because sphinx-immaterial will add an
	#   additional `/` to the url which must be ignored by the sharepoint
	#   server.
	# - The "title" is just the folder name (dev, or MAJOR.MINOR release)
	# - Aliases are used to check if the current documentation page is the
	#   latest release yes or no.
	#   - The latest release will get the alias "latest"
	#   - We always include the folder name. This is required due to the way
	#     that sphinx-immaterial checks which version the current page is for.
	#     This is checked (in javascript) by iterating over all versions.js
	#     entries and selecting the one which matches:
	# 		- $redirect_url == $base_url
	# 		- or $base_url / .. / $alias matches for any alias in aliases
	#     The redirect_url is constructed from the version, but that will
	#	  include a "/index.html#" so doesn't match the $base_url. By
	#	  including an alias with the folder name we can still let this
	#	  algorithm succeed.
	Get-ChildItem -Path "${sharepoint_root}\${hli}" -Directory -Name | Foreach-Object {
		$aliases = "[`"$_`"]"
        if ($_ -eq $latest) { 
            $aliases = "[`"$_`", `"latest`"]"
        }
		$versions.Add("    {`"version`": `"${_}/index.html#`", `"title`": `"$_`", `"aliases`": $aliases}")
	}
	Write-Debug "Latest documentation version: $latest"
	$versions_content = "[`n" + ($versions -Join ",`n") + "`n]"

	# Deploy versions.js
	Set-Content -Path "${sharepoint_root}\${hli}\versions.json" -Value $versions_content
	Set-Content -Path "${sharepoint_root}\${hli}\versions.js" -Value $versions_content
	
	# Deploy redirect html page
	$latest_html_content = "<!DOCTYPE HTML>
<html lang=`"en`">
    <head>
        <meta charset=`"utf-8`">
        <meta http-equiv=`"refresh`" content=`"0; url=${latest_url}`" />
        <link rel=`"canonical`" href=`"${latest_url}`" />
    </head>
    <body>
        <p>If this page does not refresh automatically, then please direct your browser to
            <a href=`"${latest_url}`">our latest docs</a>.
        </p>
    </body>
</html>"
	Set-Content -Path "${sharepoint_root}\${hli}\latest.html" -Value $latest_html_content

	Get-ChildItem -Path "${sharepoint_root}\${hli}"
}

Net use X: /delete