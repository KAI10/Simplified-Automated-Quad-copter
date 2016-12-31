# usage: bash tohex.sh filename
name=$(echo $1 | cut -f 1 -d '.')

#compiles and creates .elf file
avr-gcc -g -Os -mmcu=atmega32 -c $name'.c'

#creates .o file
#avr-gcc -g -mmcu=atmega32 -Wl,-Map,$name'.map' -o $name'.elf' $name'.o' #creates the map file also
avr-gcc -g -mmcu=atmega32 -o $name'.elf' $name'.o'

#creating hex file
avr-objcopy -j .text -j .data -O ihex $name'.elf' $name'.hex'
