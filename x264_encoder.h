#pragma once
#include <thread>
#include <x264.h>
#include <assert.h>

namespace x264 {

    class encoder_base {
        struct picture {
            picture(int width, int height, int csp) {
                assert(x264_picture_alloc(&pic, csp, width, height) >= 0);
            }

            ~picture() {
                x264_picture_clean(&pic);
            }

            x264_picture_t pic;
        };
    public:
        encoder_base(int width, int height, int fps = 30, int csp = X264_CSP_I420) : pic_in(width, height, csp) {
            x264_param_t param;
            assert(x264_param_default_preset(&param, "ultrafast", "zerolatency") >= 0);
            param.i_width = width;
            param.i_height = height;

            param.i_fps_num = fps;
            param.i_fps_den = 1;

            param.i_csp = csp;
            
            param.i_threads = std::thread::hardware_concurrency();
            
            param.b_repeat_headers = 1;
            
            param.b_vfr_input = 0;
            param.b_annexb = 1;

            assert(x264_param_apply_profile(&param, "baseline") >= 0);

            assert(h_x264 = x264_encoder_open(&param));

            m_width = width;
            m_height = height;
        }

        virtual ~encoder_base() {
            flush();
            x264_encoder_close(h_x264);
        }

        int width() const { return m_width; }
        int height() const { return m_height; }

        virtual bool load_yuv(uint8_t *Y, uint8_t *U, uint8_t *V) = 0;
        virtual void save_payload(uint8_t *payload, int payload_size) = 0;

        void encode() {
            x264_nal_t *nal;
            int nal_size;
            int frame_size;
            x264_picture_t pic_out;

            if (load_yuv(pic_in.pic.img.plane[0], pic_in.pic.img.plane[1], pic_in.pic.img.plane[2])) {
                assert((frame_size = x264_encoder_encode(h_x264, &nal, &nal_size, &pic_in.pic, &pic_out)) >= 0);
                if (frame_size) {
                    save_payload(nal->p_payload, frame_size);
                }
            }
        }

        void flush() {
            x264_nal_t *nal;
            int nal_size;
            int frame_size;
            x264_picture_t pic_out;

            while (x264_encoder_delayed_frames(h_x264)) {
                assert((frame_size = x264_encoder_encode(h_x264, &nal, &nal_size, nullptr, &pic_out)) >= 0);
                if (frame_size) {
                    save_payload(nal->p_payload, frame_size);
                }
            }
        }

    private:
        int m_width, m_height;
        x264_t *h_x264;
        picture pic_in;
    };

}
