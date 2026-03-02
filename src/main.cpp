#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cassert>

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

int main(){
    const size_t window_height = 512;
    const size_t window_width = 512;
    std::vector<uint32_t> frame_buffer(window_width*window_height);
    fill_gradient(frame_buffer,window_width,window_height);
    drop_ppm_image("./out.ppm",frame_buffer,window_width,window_height);
    return 0;
}

