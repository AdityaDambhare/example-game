#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstdint>
#include <cassert>

#define _USE_MATH_DEFINES
#include <math.h>


//return a color value from r,g,b and alpha values
uint32_t pack_color(const uint8_t r,const uint8_t g,const uint8_t b,const uint8_t a = 255){
    return (a<<24)+(b<<16)+(g<<8) + r;
}

//decompose a color value to r,g,b and alpha values
void unpack_color(uint32_t& color,uint8_t& r,uint8_t& g,uint8_t& b,uint8_t& a){
    r = color & 255;
    g = (color >> 8) & 255;
    b = (color >> 16) & 255;
    a = (color >> 24) & 255;
}
//fill a buffer with  color gradients
void fill_gradient(std::vector<uint32_t>&buffer,const size_t width,const size_t height){
    assert(buffer.size()==width*height);
    for(size_t i =0;i<width;i++){
        for(size_t j =0;j<height;j++){
            uint8_t r = 255*j/float(height);
            uint8_t g = 255*i/float(width);
            uint8_t b = 0;
            buffer[i+j*width] = pack_color(r,g,b);
        }
    }
}
//write buffer to .ppm image
void drop_ppm_image(std::string filename,std::vector<uint32_t>&buffer,const size_t width,const size_t height){
    assert(buffer.size()==width*height);
    std::ofstream ofs;
    ofs.open(filename, std::ofstream::out | std::ofstream::binary);//create stream
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for (size_t i = 0; i < height*width; ++i) {
        uint8_t r, g, b, a;
        unpack_color(buffer[i], r, g, b, a);
        ofs << static_cast<char>(r) << static_cast<char>(g) << static_cast<char>(b);//write to stream
    }
    ofs.close();
}

void draw_rectangle(std::vector<uint32_t>&image,const size_t image_h,const size_t image_w,
    const size_t x,const size_t y,const size_t rec_h,const size_t rec_w, const uint32_t color)
{
    assert(image.size()==image_h*image_w);
    for(int i  = 0;i<rec_w;i++){
        for(int j = 0;j<rec_h;j++){
            const size_t rx = x+i;
            const size_t ry = y+j;
            if(rx>=image_w||ry>=image_h){
                continue;
            }
            image[ry*image_w+rx] = color;
        }
    }
}

int main(){
    const size_t window_height = 512;
    const size_t window_width = 1024;
    std::vector<uint32_t> frame_buffer(window_width*window_height,pack_color(255,255,255));
    float player_x = 2.3;
    float player_y = 2.345;
    float player_q = 1.523;//direction
    const float fov = M_PI/3.;//field of view

    const size_t map_w = 16; // map width
    const size_t map_h = 16; // map height
    const char map[] = "0000222222220000"\
                       "1              0"\
                       "1     111111   0"\
                       "1     0        0"\
                       "0     0  1110000"\
                       "0     3        0"\
                       "0   10000      0"\
                       "0   0   11100  0"\
                       "0   0   0      0"\
                       "0   0   1  00000"\
                       "0       1      0"\
                       "2       1      0"\
                       "0       0      0"\
                       "0 0000000      0"\
                       "0              0"\
                       "0002222222200000"; // our game map
    assert(sizeof(map) == map_w*map_h+1); // +1 for the null terminated string

    const size_t rect_w = window_width/(map_w*2);
    const size_t rect_h = window_height/map_h;

  
    for(size_t frame = 0;frame<90;frame++){
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(5) << frame << ".ppm";
        player_q += 2*M_PI/360;
        frame_buffer = std::vector<uint32_t>(window_height*window_width,pack_color(255,255,255));
        for (size_t j=0; j<map_h; j++) { // draw the map
            for (size_t i=0; i<map_w; i++) {
                if (map[i+j*map_w]==' ') continue; // skip empty spaces
                size_t rect_x = i*rect_w;
                size_t rect_y = j*rect_h;
                draw_rectangle(frame_buffer, window_height, window_width, rect_x, rect_y, rect_h, rect_w,
                    pack_color(0, 255, 255));
            }
        }
        for (size_t i=0; i<window_width/2; i++) { // draw the visibility cone
            float angle = player_q-fov/2 + fov*i/float(window_width/2);
            //trace ray
            for (float t=0; t<20; t+=.01) {
                float cx = player_x + t*cos(angle);
                float cy = player_y + t*sin(angle);

                size_t pix_x = cx*rect_w;
                size_t pix_y = cy*rect_h;
                frame_buffer[pix_x + pix_y*window_width] = pack_color(160,160,160);

                if(map[int(cx)+int(cy)*map_w]!=' '){//ray touches a wall
                        size_t col_h = window_height/(t*cos(angle-player_q));
                        draw_rectangle(frame_buffer,window_height,window_width,window_width/2+i,window_height/2-col_h/2,
                        col_h,1,pack_color(0,255,255)
                        );
                        break;
                }

            }
        }
        drop_ppm_image("./images/"+ss.str(),frame_buffer,window_width,window_height);
    }
 
    return 0;
}

#undef _USE_MATH_DEFINES