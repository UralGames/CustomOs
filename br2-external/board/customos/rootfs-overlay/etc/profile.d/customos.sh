# CustomOS shell defaults.
export EDITOR=nano
export PAGER=less
export LANG=${LANG:-en_US.UTF-8}
if [ -n "$BASH_VERSION" ] && [ -t 1 ]; then
    PS1='\[\e[1;36m\]\u@\h\[\e[0m\]:\[\e[1;34m\]\w\[\e[0m\]\$ '
fi
