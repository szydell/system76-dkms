#!/usr/bin/env bash
#
# Parse debian/changelog and set package version same as on PopOS! and Ubuntu
# 
# ----------------------------------------------------------------------------
# 2021-01-09 Marcin Szydelski
#		init

# config
export outdir="$(pwd)/.rpkg-build"

readonly PACKAGE="system76-dkms"


# verification
[ -f debian/changelog ] || { echo "No debian/changelog found."; exit 1; }

# fetch upstream
git fetch upstream
# merge
git checkout master
git merge upstream/master -m "fetch upstream" --log


[ -d ${outdir} ] && rm -rf ${outdir}

version_in_changelog=$(grep -E "${PACKAGE} \([[:digit:]]+\.[[:digit:]]+\.[[:digit:]]+" debian/changelog | head -1)
_tmp=${version_in_changelog%%)*}
_tmp=${_tmp%%~*}
version=${_tmp##*\(}

_tmp=$(git tag --list "${PACKAGE}"-"$version"-'*' | sort -n -t '-' -k 4 -r | head -1)
release=${_tmp##*-}

if [ "z$release" == "z" ]; then
	release=1
else
	if ! [[ "$release" =~ ^[0-9]+$ ]]; then
		echo "Release should be a number"
		exit 2
	fi
	# increment release number
	((release++))
fi

# as a workaround set static version in spec file
sed -i "s#^Version:    .*#Version:    $version#" "${PACKAGE}.spec.rpkg"
sed -i "s#^Release:    .*#Release:    $release#" "${PACKAGE}.spec.rpkg"
git commit -m"bump Version to: $version-$release" "${PACKAGE}.spec.rpkg"

#test & build srpm
mkdir "$outdir"
rpkg local --outdir="$outdir" || { echo "rpkg local failed"; exit 4; }

# rpkg tag
rpkg tag --version="$version"  --release="$release"

srpm="$(ls .rpkg-build/${PACKAGE}-*.src.rpm)"

# publish / build oficially

copr-cli build system76 "$srpm" || { echo "Copr build failed"; exit 5; }

# store in repo
git push || { echo "Git push failed"; exit 6; }
git push --tags || { echo "Git push --tags failed"; exit 6; }

# clear

if [ -d "$outdir" ]; then
	rm -rf "$outdir"
fi
