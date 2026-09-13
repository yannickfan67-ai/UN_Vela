#include <stdio.h>
#include "vela_image.h"

int main(void){
    static const unsigned char ppm[]={ 'P','6','\n','2',' ','1','\n','2','5','5','\n', 0x11,0x22,0x33, 0xAA,0xBB,0xCC };
    unsigned char rgb[6];VelaImageInfo i;
    if(!vela_image_probe(ppm,sizeof(ppm),&i)||i.width!=2||i.height!=1)return 1;
    if(!vela_image_decode_rgb24(ppm,sizeof(ppm),rgb,sizeof(rgb),&i))return 2;
    if(rgb[0]!=0x11||rgb[1]!=0x22||rgb[2]!=0x33||rgb[3]!=0xAA||rgb[4]!=0xBB||rgb[5]!=0xCC)return 3;

    unsigned char bad_bmp[54]={0};
    bad_bmp[0]='B';bad_bmp[1]='M';
    bad_bmp[14]=40;
    bad_bmp[18]=1;
    bad_bmp[22]=0x00;bad_bmp[23]=0x00;bad_bmp[24]=0x00;bad_bmp[25]=0x80;
    bad_bmp[26]=1;
    bad_bmp[28]=24;
    if(vela_image_probe(bad_bmp,sizeof(bad_bmp),&i))return 4;

    puts("UN_Vela image decoder smoke passed");
    return 0;
}
