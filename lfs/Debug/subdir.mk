################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../API.c \
../Compactador.c \
../Dump.c \
../Helper.c \
../Lissandra.c 

OBJS += \
./API.o \
./Compactador.o \
./Dump.o \
./Helper.o \
./Lissandra.o 

C_DEPS += \
./API.d \
./Compactador.d \
./Dump.d \
./Helper.d \
./Lissandra.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2019-1c-bugbusters/nuestro_lib" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


