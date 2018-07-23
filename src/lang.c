#include <stdlib.h>
#include <string.h>

#include "lang.h"
#include "util.h"

const lang_spec_t langs[] = {
    { "actionscript", { "as", "mxml" } },
    { "ada", { "ada", "adb", "ads" } },
    { "am", { "^Makefile\\.am", "^configure\\.ac" } },
    { "asciidoc", { "adoc", "ad", "asc", "asciidoc" } },
    { "apl", { "apl" } },
    { "asm", { "asm", "s", "S" } },
    { "batch", { "bat", "cmd" } },
    { "bb", { "bb", "bbappend", "bbclass", "inc", "conf" } },
    { "bitbake", { "bb", "bbappend", "bbclass", "inc", "conf" } },
    { "bro", { "bro", "bif" } },
    { "cc", { "c", "h", "xs" } },
    { "cfmx", { "cfc", "cfm", "cfml" } },
    { "chpl", { "chpl" } },
    { "clojure", { "clj", "cljs", "cljc", "cljx" } },
    { "cmake", { "^CMakeLists\\.txt", "cmake" } },
    { "coffee", { "coffee", "cjsx" } },
    { "coq", { "coq", "g", "v" } },
    { "cpp", { "cpp", "cc", "C", "cxx", "m", "hpp", "hh", "h", "H", "hxx", "tpp" } },
    { "crystal", { "cr", "ecr" } },
    { "csharp", { "cs" } },
    { "css", { "css" } },
    { "cython", { "pyx", "pxd", "pxi" } },
    { "delphi", { "pas", "int", "dfm", "nfm", "dof", "dpk", "dpr", "dproj", "groupproj", "bdsgroup", "bdsproj" } },
    { "dlang", { "d", "di" } },
    { "dot", { "dot", "gv" } },
    { "dts", { "dts", "dtsi" } },
    { "ebuild", { "ebuild", "eclass" } },
    { "elisp", { "el" } },
    { "elixir", { "ex", "eex", "exs" } },
    { "elm", { "elm" } },
    { "erlang", { "erl", "hrl" } },
    { "factor", { "factor" } },
    { "fortran", { "f", "f77", "f90", "f95", "f03", "for", "ftn", "fpp" } },
    { "fsharp", { "fs", "fsi", "fsx" } },
    { "gettext", { "po", "pot", "mo" } },
    { "glsl", { "vert", "tesc", "tese", "geom", "frag", "comp" } },
    { "go", { "go" } },
    { "groovy", { "groovy", "gtmpl", "gpp", "grunit", "gradle" } },
    { "haml", { "haml" } },
    { "handlebars", { "hbs" } },
    { "haskell", { "hs", "lhs" } },
    { "haxe", { "hx" } },
    { "hh", { "h" } },
    { "html", { "htm", "html", "shtml", "xhtml" } },
    { "idris", { "idr", "ipkg", "lidr" } },
    { "ini", { "ini" } },
    { "ipython", { "ipynb" } },
    { "isabelle", { "thy" } },
    { "j", { "ijs" } },
    { "jade", { "jade" } },
    { "java", { "java", "properties" } },
    { "jinja2", { "j2" } },
    { "js", { "es6", "js", "jsx", "vue" } },
    { "json", { "json" } },
    { "jsp", { "jsp", "jspx", "jhtm", "jhtml", "jspf", "tag", "tagf" } },
    { "julia", { "jl" } },
    { "kotlin", { "kt" } },
    { "less", { "less" } },
    { "liquid", { "liquid" } },
    { "lisp", { "lisp", "lsp" } },
    { "log", { "log" } },
    { "lua", { "lua" } },
    { "m4", { "m4" } },
    { "make", { "^Makefile(\\.[^/]+)?", "Makefiles", "mk", "mak", "make" } },
    { "mako", { "mako" } },
    { "markdown", { "markdown", "mdown", "mdwn", "mkdn", "mkd", "md" } },
    { "mason", { "mas", "mhtml", "mpl", "mtxt" } },
    { "matlab", { "m" } },
    { "mathematica", { "m", "wl" } },
    { "md", { "markdown", "mdown", "mdwn", "mkdn", "mkd", "md" } },
    { "mercury", { "m", "moo" } },
    { "naccess", { "asa", "rsa" } },
    { "nim", { "nim" } },
    { "nix", { "nix" } },
    { "objc", { "m", "h" } },
    { "objcpp", { "mm", "h" } },
    { "ocaml", { "ml", "mli", "mll", "mly" } },
    { "octave", { "m" } },
    { "org", { "org" } },
    { "parrot", { "pir", "pasm", "pmc", "ops", "pod", "pg", "tg" } },
    { "pdb", { "pdb" } },
    { "perl", { "pl", "pm", "pm6", "pod", "t" } },
    { "php", { "php", "phpt", "php3", "php4", "php5", "phtml" } },
    { "pike", { "pike", "pmod" } },
    { "plist", { "plist" } },
    { "plone", { "pt", "cpt", "metadata", "cpy", "py", "xml", "zcml" } },
    { "proto", { "proto" } },
    { "pug", { "pug" } },
    { "puppet", { "pp" } },
    { "py", { "py" } },
    { "python", { "py" } },
    { "qml", { "qml" } },
    { "racket", { "rkt", "ss", "scm" } },
    { "restructuredtext", { "rst" } },
    { "rs", { "rs" } },
    { "r", { "r", "R", "Rmd", "Rnw", "Rtex", "Rrst" } },
    { "rdoc", { "rdoc" } },
    { "ruby", { "rb", "rhtml", "rjs", "rxml", "erb", "rake", "spec" } },
    { "rust", { "rs" } },
    { "salt", { "sls" } },
    { "sass", { "sass", "scss" } },
    { "scala", { "scala" } },
    { "scheme", { "scm", "ss" } },
    { "sh", { "sh", "bash", "csh", "tcsh", "ksh", "zsh", "fish" } },
    { "shell", { "sh", "bash", "csh", "tcsh", "ksh", "zsh", "fish" } },
    { "smalltalk", { "st" } },
    { "sml", { "sml", "fun", "mlb", "sig" } },
    { "sql", { "sql", "ctl" } },
    { "stylus", { "styl" } },
    { "swift", { "swift" } },
    { "tcl", { "tcl", "itcl", "itk" } },
    { "tex", { "tex", "cls", "sty" } },
    { "thrift", { "thrift" } },
    { "tla", { "tla" } },
    { "tt", { "tt", "tt2", "ttml" } },
    { "toml", { "toml" } },
    { "ts", { "ts", "tsx" } },
    { "twig", { "twig" } },
    { "vala", { "vala", "vapi" } },
    { "vb", { "bas", "cls", "frm", "ctl", "vb", "resx" } },
    { "velocity", { "vm", "vtl", "vsl" } },
    { "verilog", { "v", "vh", "sv" } },
    { "vhdl", { "vhd", "vhdl" } },
    { "vim", { "vim" } },
    { "wix", { "wxi", "wxs" } },
    { "wsdl", { "wsdl" } },
    { "wadl", { "wadl" } },
    { "xml", { "xml", "dtd", "xsl", "xslt", "ent", "tld", "plist" } },
    { "yaml", { "yaml", "yml" } },
};

size_t get_lang_count() {
    return sizeof(langs) / sizeof(lang_spec_t);
}

char *make_lang_regex(size_t *ext_index, size_t len) {
    char *ext_regex = NULL;
    char *name_regex = NULL;
    size_t ext_size, name_size;
    size_t ext_pos, name_pos;
    size_t ext_count = 0, name_count = 0;
    size_t i;

    // to avoid repeating the '\.' every time for an extension, generate separate regexes
    // for the extensions and filename patterns
    // note: the regex from this function is matched against the full path name (relative to the base_path
    // specified on the command line) and not just the filename. When individual files are given on the command
    // line, the file search regex isn't even checked, so for filename checks we can always match against a '/'
    // before the pattern to keep this from matching everything in a directory which matches a filename pattern
    ext_pos = ag_dsprintf(&ext_regex, &ext_size, 0, "\\.(");
    name_pos = ag_dsprintf(&name_regex, &name_size, 0, "/(");
    for (i = 0; i < len; i++) {
        size_t j = 0;
        const char *ext = langs[ext_index[i]].extensions[j];
        while (ext != NULL) {
            if (ext[0] == '^') {
                name_pos += ag_dsprintf(&name_regex, &name_size, name_pos, "%s|", ext + 1);
                name_count++;
            } else {
                ext_pos += ag_dsprintf(&ext_regex, &ext_size, ext_pos, "%s|", ext);
                ext_count++;
            }

            // the list of extensions will be null-terminated only if there are less than MAX_EXTENSIONS
            // defined for a particular filetype, therefore make sure we don't accidentally overflow
            if (++j >= MAX_EXTENSIONS)
                break;
            ext = langs[ext_index[i]].extensions[j];
        }
    }

    // drop the trailing '|' in both regexes
    if (ext_count > 0)
        ext_pos--;
    if (name_count > 0)
        name_pos--;

    // finish off the regexes
    ext_pos += ag_dsprintf(&ext_regex, &ext_size, ext_pos, ")$");
    name_pos += ag_dsprintf(&name_regex, &name_size, name_pos, ")$");

    // if only one type of extension, just return that. otherwise combine them
    if (name_count == 0) {
        free(name_regex);
        return ext_regex;
    } else if (ext_count == 0) {
        free(ext_regex);
        return name_regex;
    } else {
        size_t outsize = strlen(name_regex) + strlen(ext_regex) + 2;
        char *outbuf = ag_malloc(outsize);
        snprintf(outbuf, outsize, "%s|%s", ext_regex, name_regex);
        free(name_regex);
        free(ext_regex);
        return outbuf;
    }
}
