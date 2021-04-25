# Copyright 1999-2021 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=7

inherit cmake

DESCRIPTION="A multi-platform GUI application that helps browse and search through long and complex log files. It is designed with programmers and system administrators in mind and can be seen as a graphical, interactive combination of grep, less, and tail"
HOMEPAGE="https://klogg.filimonov.dev"
SRC_URI="https://github.com/variar/klogg/archive/refs/tags/v${PV}.tar.gz"

LICENSE="GPL-3+"
SLOT="0"
KEYWORDS="~amd64 ~x86"

DEPEND="
	dev-qt/qtcore:5
	dev-qt/qtgui:5
	dev-qt/qtwidgets:5
	dev-qt/qtnetwork:5
	dev-qt/qtxml:5
	dev-qt/qtconcurrent:5
"

RDEPEND="${DEPEND} 
	x11-themes/hicolor-icon-theme
"

BDEPEND="
	>=dev-util/cmake-3.12
"

IUSE="+sentry lto"

src_prepare() {
	sed -e 's|share/doc/klogg|${CMAKE_INSTALL_DOCDIR}|' -i ${S}/CMakeLists.txt || die "sed CMAKE_INSTALL_DOCDIR"
	sed -e 's|TBB_INSTALL_LIBRARY_DIR lib|TBB_INSTALL_LIBRARY_DIR ${CMAKE_INSTALL_LIBDIR}|' -i ${S}/3rdparty/tbb/CMakeLists.txt || die "sed TBB_INSTALL_LIBRARY_DIR"

	eapply_user
	cmake_src_prepare
}

src_configure() {
	local cmakeopts="-DWARNINGS_AS_ERRORS=OFF -DDISABLE_WERROR=ON"

	if use sentry; then
		cmakeopts+=" -DKLOGG_USE_SENTRY=ON"
	else
		cmakeopts+=" -DKLOGG_USE_SENTRY=OFF"
	fi
	
	if use lto; then
		cmakeopts+=" -DUSE_LTO=ON"
	else
		cmakeopts+=" -DUSE_LTO=OFF"
	fi

	local mycmakeargs=(
		${cmakeopts}
	)
	export KLOGG_VERSION=${PV}.0.813
	cmake_src_configure
}

src_compile() {
	cmake_src_compile ${PN}
}

pkg_postinst() {
	xdg_desktop_database_update
}

pkg_postrm() {
	xdg_desktop_database_update
}
