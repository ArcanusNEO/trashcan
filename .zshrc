setopt rmstarsilent
export ZSH=/usr/share/oh-my-zsh
export ZSH_THEME=simple
export DISABLE_AUTO_UPDATE=true
export DISABLE_MAGIC_FUNCTIONS=true
export ZSH_CACHE_DIR=$HOME/.cache/oh-my-zsh
if [[ ! -d $ZSH_CACHE_DIR ]]; then
  mkdir -p $ZSH_CACHE_DIR
fi
plugins=(
  git
)
source "$ZSH/oh-my-zsh.sh"
source /usr/share/zsh/plugins/zsh-autosuggestions/zsh-autosuggestions.zsh
source /usr/share/zsh/plugins/zsh-syntax-highlighting/zsh-syntax-highlighting.zsh
PROMPT='%(?.%{$fg[green]%}.%{$fg[red]%})%?%{$fg_bold[blue]%}|%{$fg[green]%}%C%{$fg_bold[blue]%}$(git_prompt_info)%{$fg_bold[cyan]%}'
if [[ $(id -u) -eq 0 ]]; then
  PROMPT+='# %{$reset_color%}'
else
  PROMPT+='$ %{$reset_color%}'
fi
export PROMPT

#umask 002
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
alias mkddir='mkdir Applications Comics Desktop Documents Downloads Games Library Movies Music Pictures Public Resources Templates Temporary Videos'
alias sudo='sudo '
alias srun='run0 --background="" '
alias nvrun='nvrun '
alias brun='brun '
alias cls='printf "\033[2J\033[3J\033[1;1H"'
alias load.aocc='module load aocc && module load aocl-aocc'
alias unload.aocc='module unload aocc; module unload aocl-aocc'
alias yay='run0 --background="" --user=system -- /usr/bin/paru'
alias switcheroo='envycontrol --verbose --coolbits 24 --switch'
alias sdcv='sdcv --non-interactive --utf8-output --utf8-input --color'
export UA_OSX='Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/15.4 Safari/605.1.15'
export UA_WIN='Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/139.0.0.0 Safari/537.36 Edg/139.0.3405.102'
alias curl="curl -A'$UA_OSX'"
alias wget="wget -U'$UA_OSX'"
alias aget="aria2c -c -x16 -k1M -U'$UA_OSX'"
export EDITOR=qe
export VISUAL='emacsclient -tc -a "emacs -nw"'
alias wineinit='winetricks -q vcrun6 vcrun6sp6 gdiplus riched20 riched30 richtx32 msctf msftedit autostart_winedbg=disabled'
export WINEPREFIX="$HOME/wine/64/default"
export WINEARCH=win64
export WINEDEBUG=-all
export WINETRICKS_DOWNLOADER=wget
unset DEBUGINFOD_URLS
unalias cd &>/dev/null
export PATH="$HOME/.local/bin:"${PATH-/usr/local/sbin:/usr/sbin:/sbin:/usr/local/bin:/usr/bin:/bin:/usr/local/games:/usr/games}":/opt/bin"

if [[ -f "$HOME/.zshrc_local" ]]; then
  source "$HOME/.zshrc_local"
fi

return 0
