#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstdint>
#include <cassert>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

bool load_texture(const std::string filename, std::vector<uint32_t> &texture, size_t &text_size, size_t &text_cnt) {
    int nchannels = -1, w, h;
    unsigned char *pixmap = stbi_load(filename.c_str(), &w, &h, &nchannels, 0);
    if (!pixmap) {
        std::cerr << "Error: can not load the textures" << std::endl;
        return false;
    }

    if (4!=nchannels) {
        std::cerr << "Error: the texture must be a 32 bit image" << std::endl;
        stbi_image_free(pixmap);
        return false;
    }

    text_cnt = w/h;
    text_size = w/text_cnt;
    if (w!=h*int(text_cnt)) {
        std::cerr << "Error: the texture file must contain N square textures packed horizontally" << std::endl;
        stbi_image_free(pixmap);
        return false;
    }

    texture = std::vector<uint32_t>(w*h);
    for (int j=0; j<h; j++) {
        for (int i=0; i<w; i++) {
            uint8_t r = pixmap[(i+j*w)*4+0];
            uint8_t g = pixmap[(i+j*w)*4+1];
            uint8_t b = pixmap[(i+j*w)*4+2];
            uint8_t a = pixmap[(i+j*w)*4+3];
            texture[i+j*w] = pack_color(r, g, b, a);
        }
    }
    stbi_image_free(pixmap);
    return true;
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
std::vector<uint32_t> texture_column(const std::vector<uint32_t> &img, const size_t texsize, const size_t ntextures, const size_t texid, const size_t texcoord, const size_t column_height) {
    const size_t img_w = texsize*ntextures;
    const size_t img_h = texsize;
    assert(img.size()==img_w*img_h && texcoord<texsize && texid<ntextures);
    std::vector<uint32_t> column(column_height);
    for (size_t y=0; y<column_height; y++) {
        size_t pix_x = texid*texsize + texcoord;
        size_t pix_y = (y*texsize)/column_height;
        column[y] = img[pix_x + pix_y*img_w];
    }
    return column;
}

int main(){
    const size_t window_height = 512;
    const size_t window_width = 1024;
    std::vector<uint32_t> frame_buffer(window_width*window_height,pack_color(255,255,255));
    float player_x = 2.3;
    float player_y = 2.345;
    float player_q = 1.523;//direction
    const float fov = M_PI/3.;//field of view
    const size_t ncolors = 10;
    std::vector<uint32_t> colors(ncolors);
    for (size_t i=0; i<ncolors; i++) {
        colors[i] = pack_color(rand()%255, rand()%255, rand()%255);
    }
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

    std::vector<uint32_t> walltext; // textures for the walls
    size_t walltext_size;  // texture dimensions (it is a square)
    size_t walltext_cnt;   // number of different textures in the image
    if (!load_texture("./images/walltext.png", walltext, walltext_size, walltext_cnt)) {
        std::cerr << "Failed to load wall textures" << std::endl;
        return -1;
    }
    const size_t rect_w = window_width/(map_w*2);
    const size_t rect_h = window_height/map_h;

        for (size_t j=0; j<map_h; j++) { // draw the map
            for (size_t i=0; i<map_w; i++) {
                if (map[i+j*map_w]==' ') continue; // skip empty spaces
                size_t rect_x = i*rect_w;
                size_t rect_y = j*rect_h;
                size_t icolor =  map[i+j*map_w]-'0';
                assert(icolor<ncolors);
                draw_rectangle(frame_buffer, window_height, window_width, rect_x, rect_y, rect_h, rect_w,
                    colors[icolor]);
            }
        }
        for (size_t i=0; i<window_width/2; i++) { // draw the visibility cone
            float angle = player_q-fov/2 + fov*i/float(window_width/2);
            //trace ray
            for (float t=0; t<20; t+=.01) {
                float cx = player_x + t*cos(angle);
                float cy = player_y + t*sin(angle);

                int pix_x = cx*rect_w;
                int pix_y = cy*rect_h;
                frame_buffer[pix_x + pix_y*window_width] = pack_color(160,160,160);

                if(map[int(cx)+int(cy)*map_w]!=' '){//ray touches a wall
                        size_t icolor = map[(int)cx+(int)cy*map_w]-'0';
                        assert(icolor<ncolors);
                        size_t col_h = window_height/(t*cos(angle-player_q));
                        float hitx = cx - floor(cx+.5);
                        float hity = cy - floor(cy+.5);
                        size_t texid = map[int(cx)+int(cy)*map_w] - '0';
                        int x_textcoord = hitx*walltext_size;
                        if(std::abs(hity)>std::abs(hitx)){
                            x_textcoord = hity*walltext_size;
                        }
                        if(x_textcoord<0) x_textcoord+=walltext_size;
                        assert(x_textcoord>=0&&x_textcoord<(int)walltext_size);
                        std::vector<uint32_t> column = texture_column(walltext, walltext_size, walltext_cnt,texid,x_textcoord, 
                        col_h);
                        pix_x = window_width/2+i;
                    for (size_t j=0; j<col_h; j++) {
                        pix_y = j + window_height/2-col_h/2;
                        if (pix_y<0 || pix_y>=(int)window_height) continue;
                        frame_buffer[pix_x + pix_y*window_width] = column[j];
                    }
                        break;
                }
            }
        }

    const size_t texid = 4; // draw the 4th texture on the screen
    for (size_t i=0; i<walltext_size; i++) {
        for (size_t j=0; j<walltext_size; j++) {
            frame_buffer[i+j*window_width] = walltext[i + texid*walltext_size + j*walltext_size*walltext_cnt];}
        }
    drop_ppm_image("./out.ppm",frame_buffer,window_width,window_height); 
    return 0;
}

#undef _USE_MATH_DEFINES