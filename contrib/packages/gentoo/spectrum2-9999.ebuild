# Copyright 1999-2010 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

inherit cmake-utils git-2

EGIT_REPO_URI="git://github.com/hanzz/libtransport.git"

LICENSE="BSD"
SLOT="0"
KEYWORDS=""
IUSE=""

DEPEND=""
RDEPEND=""

src_prepare() {
    git_src_prepare
}
src_configure() {
    cmake-utils_src_configure
}
