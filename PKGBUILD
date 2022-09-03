# Maintainer: Matthias Quintern <matthiasqui@protonmail.com>
pkgname=gz-rgb
pkgver=1.0
pkgrel=1
pkgdesc="rgb control daemon"
arch=('any')
url="https://github.com/MatthiasQuintern/gz-rgb"
license=('GPL3')
depends=('openrgb')
makedepends=('git')
source=("git+${url}#branch=main")
md5sums=('SKIP')

build() {
	mkdir -p pkg
	cd "${pkgname}/src"
	git submodule update --init --recursive
	make release
	make DESTDIR="${srcdir}/pkg_tmp" install
}


package() {
	mv ${srcdir}/pkg_tmp/* ${pkgdir}
	rm -d ${srcdir}/pkg_tmp
}
