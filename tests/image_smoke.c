#include <stdio.h>
#include "vela_image.h"

int main(void){
    static const unsigned char ppm[]={ 'P','6','\n','2',' ','1','\n','2','5','5','\n', 0x11,0x22,0x33, 0xAA,0xBB,0xCC };
    unsigned char rgb[6];VelaImageInfo i;
    if(!vela_image_probe(ppm,sizeof(ppm),&i)||i.width!=2||i.height!=1)return 1;
    if(!vela_image_decode_rgb24(ppm,sizeof(ppm),rgb,sizeof(rgb),&i))return 2;
    if(rgb[0]!=0x11||rgb[1]!=0x22||rgb[2]!=0x33||rgb[3]!=0xAA||rgb[4]!=0xBB||rgb[5]!=0xCC)return 3;
    puts("UN_Vela image decoder smoke passed");
    return 0;
}
