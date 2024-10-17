################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/client_server/fileSystem.c \
../src/client_server/utils.c 

C_DEPS += \
./src/client_server/fileSystem.d \
./src/client_server/utils.d 

OBJS += \
./src/client_server/fileSystem.o \
./src/client_server/utils.o 


# Each subdirectory must supply rules for building sources it contributes
src/client_server/%.o: ../src/client_server/%.c src/client_server/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src-2f-client_server

clean-src-2f-client_server:
	-$(RM) ./src/client_server/fileSystem.d ./src/client_server/fileSystem.o ./src/client_server/utils.d ./src/client_server/utils.o

.PHONY: clean-src-2f-client_server

