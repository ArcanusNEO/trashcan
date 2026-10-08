# Bash completion for brun (a transparent command runner).
# Install as ~/.local/share/bash-completion/completions/brun or source this file.

_brun_completion() {
    COMPREPLY=()

    # bash-completion handles both the command name and its own arguments.
    # Unlike _command, offset 1 does not interpret any arguments as brun flags.
    if declare -F _command_offset >/dev/null 2>&1; then
        _command_offset 1
        return
    fi

    # Without bash-completion, offer executable names/paths for argument 1.
    # For subsequent arguments, -o default below provides filename completion.
    if (( COMP_CWORD == 1 )); then
        local cur=${COMP_WORDS[COMP_CWORD]} match
        if [[ $cur == */* ]]; then
            while IFS= read -r match; do
                [[ -d $match || -x $match ]] && COMPREPLY+=("$match")
            done < <(compgen -f -- "$cur")
            if type compopt >/dev/null 2>&1; then
                compopt -o filenames 2>/dev/null || :
            fi
        else
            while IFS= read -r match; do
                COMPREPLY+=("$match")
            done < <(compgen -c -- "$cur")
        fi
    fi
}

complete -o bashdefault -o default -F _brun_completion brun
