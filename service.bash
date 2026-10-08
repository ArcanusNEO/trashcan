# Bash completion for the Slackware service wrapper.
# Install as /usr/share/bash-completion/completions/service

_slackware_service_complete() {
  local cur rcpath dir script name
  local -a dirs choices
  local -A seen=()

  cur=${COMP_WORDS[COMP_CWORD]}

  case $COMP_CWORD in
    1)
      # These are the only standalone wrapper options.
      choices=(--list --help -l -h list help)
      if [[ $cur != -* ]]; then
        rcpath=${RCPATH:-"$HOME/.local/etc/rc.d:/usr/local/etc/rc.d:/etc/rc.d"}
        IFS=: read -r -a dirs <<< "$rcpath"

        for dir in "${dirs[@]}"; do
          [[ -n $dir && -d $dir ]] || continue
          for script in "$dir"/rc.*; do
            [[ -f $script ]] || continue
            case ${script##*/} in
              rc.[0-9]|rc.S|rc.M|rc.K|rc.local|rc.local_shutdown)
                continue ;;
            esac

            name=${script##*/rc.}
            [[ $name =~ ^[a-zA-Z0-9_.+-]+$ ]] || continue
            [[ ${seen[$name]+exists} ]] && continue
            seen[$name]=1

            if [[ $cur == rc.* ]]; then
              choices+=("rc.$name")
            else
              choices+=("$name")
            fi
          done
        done
      fi
      ;;
    2)
      case ${COMP_WORDS[1]} in
        --list|-l|list|--help|-h|help) return ;;
      esac
      choices=(start stop restart reload force-reload status enable disable)
      ;;
    *)
      # Additional arguments belong to the rc script, not the wrapper.
      compopt -o default
      return ;;
  esac

  COMPREPLY=( $(compgen -W "${choices[*]}" -- "$cur") )
}

complete -F _slackware_service_complete service
