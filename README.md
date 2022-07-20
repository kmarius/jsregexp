# jsregexp

Uses `libregexp` from Fabrice Bellard's QuickJS.

## Installation

If you intend to use `jsregexp` in neovim you can place

    use_rocks 'jsregexp'
    
in the function passed to `packer.startup`.

To install `jsregexp` locally with luarocks, run

    luarocks --local --lua-version 5.1 install jsregexp

This will place the compiled module in a subdirectory of `$HOME/.luarocks`.
