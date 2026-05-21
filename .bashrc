#If not running interactively, don't do anything
# [[ $- != *i* ]] && return

# umask 002
export PS1='[$?][\u@\h \W]\$ '
alias ls="ls --color=auto"
alias l='ls -Fhla'
alias ll='ls -FhlaSr'
alias lt='ls -Fhlatr'
alias lx='ls -FhlaX'
alias lv='ls -Fhlav'
alias diff='diff --color=auto'
alias grep='grep --color=auto'
alias ip='ip -color=auto'
alias j='cd'
alias jp='cd $(pwd -P)'
alias vi='vim'
alias ec='emacsclient -tc -a "emacs -nw"'
alias md='mkdir -p'
alias mkpdir='mkdir bin doc etc games include info lib libexec local man misc share src'
alias mkddir='mkdir Applications Comics Desktop Documents Downloads Games Library Movies Music Pictures Projects Public Resources Templates Temporary Videos'
alias sudo='sudo '
alias nvrun='nvrun '
alias brun='brun '
alias cls='printf "\033[2J\033[3J\033[1;1H"'
alias switcheroo='envycontrol --verbose --coolbits 24 --switch'
export EDITOR=vi
# export VISUAL='emacsclient -tc -a "emacs -nw"'
export WINEPREFIX="$HOME/wine/default"
export WINEARCH=win64
export WINEDEBUG=-all
export WINETRICKS_DOWNLOADER=wget
unset DEBUGINFOD_URLS
unalias cd &>/dev/null
PATH="$HOME/.local/bin:"${PATH-/usr/local/sbin:/usr/sbin:/sbin:/usr/local/bin:/usr/bin:/bin:/usr/local/games:/usr/games}":/opt/bin"
PATH="${PATH//:$HOME\/.local\/bin/}"
PATH="${PATH//\/opt\/bin:/}"
export PATH

return 0
