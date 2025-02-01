
{ pkgs }: {
	deps = [
		pkgs.clang_15
		pkgs.ccls
		pkgs.gdb
		pkgs.gnumake
	];
}
