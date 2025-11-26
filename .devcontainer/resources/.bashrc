case $- in
    *i*) ;;
      *) return;;
esac

HISTCONTROL="ignoreboth"
shopt -s "histappend"
HISTSIZE=1000
HISTFILESIZE=2000

shopt -s "checkwinsize"

[ -x "/usr/bin/lesspipe" ] && eval "$(SHELL="/bin/sh" lesspipe)"

if [ -z "${debian_chroot:-}" ] && [ -r "/etc/debian_chroot" ]; then
    debian_chroot=$(cat "/etc/debian_chroot")
fi

case "${TERM}" in
    xterm-color|*-256color) color_prompt="yes";;
esac

if [ -n "${force_color_prompt}" ]; then
    if [ -x "/usr/bin/tput" ] && tput "setaf" 1 >&/dev/null; then
        color_prompt="yes"
    else
        color_prompt=""
    fi
fi

if [ "${color_prompt}" = "yes" ]; then
    PS1='${debian_chroot:+($debian_chroot)}\[\033[01;32m\]\u@\h\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$ '
else
    PS1='${debian_chroot:+($debian_chroot)}\u@\h:\w\$ '
fi
unset color_prompt force_color_prompt

case "${TERM}" in
    xterm*|rxvt*)
        PS1="\[\e]0;${debian_chroot:+(${debian_chroot})}\u@\h: \w\a\]${PS1}"
        ;;
    *)
        ;;
esac

if [ -x "/usr/bin/dircolors" ]; then
    if [ -r "${HOME}/.dircolors" ]; then
        eval "$(dircolors -b "${HOME}/.dircolors")"
    else
        eval "$(dircolors -b)"
    fi
    alias ls='ls --color=auto'
    alias grep='grep --color=auto'
    alias fgrep='fgrep --color=auto'
    alias egrep='egrep --color=auto'
fi

if [ -f "${HOME}/.bash_aliases" ]; then
    . "${HOME}/.bash_aliases"
fi

if ! shopt -oq "posix"; then
    if [ -f "/usr/share/bash-completion/bash_completion" ]; then
        . "/usr/share/bash-completion/bash_completion"
    elif [ -f "/etc/bash_completion" ]; then
        . "/etc/bash_completion"
    fi
fi

if [[ $- == *i* ]] && command -v "show_env" >/dev/null 2>&1; then
    show_env
fi
