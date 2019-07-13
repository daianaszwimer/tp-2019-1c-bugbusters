################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../nuestro_lib/nuestro_lib.c 

OBJS += \
./nuestro_lib/nuestro_lib.o 

C_DEPS += \
./nuestro_lib/nuestro_lib.d 


# Each subdirectory must supply rules for building sources it contributes
nuestro_lib/nuestro_lib.o: ../nuestro_lib/nuestro_lib.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"nuestro_lib/nuestro_lib.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


