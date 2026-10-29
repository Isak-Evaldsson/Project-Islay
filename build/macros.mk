ifndef $(_BUILD_MACROS_MK)
_BUILD_MACROS_MK := "included"

#
# For storing various utility macros used throughout the build system
#

# Check if variables provided in $1 are set and all have non-empty values,
# die with an error otherwise. A custom error message can be passed in $2
#
# By Eldar Abusalimov at stackoverflow, https://stackoverflow.com/questions/10858261/how-to-abort-makefile-if-variable-not-set
check_defined = \
	$(strip $(foreach 1,$1, \
		$(call __check_defined,$1,$(strip $(value 2)))))
__check_defined = \
	$(if $(value $1),, \
		$(error Undefined $1$(if $2, ($2))))

# Checks if the file provided as first arguments exist 
check_file = $(if $(wildcard $1),, \
		$(error No such file $1))

endif