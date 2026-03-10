/*
 * Copyright (c) 2001 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <cstdio>
#include <fmt/format.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
};
#include "VisionLog.h"

static void Encode(AVCodecContext* encCtx, AVFrame* frame, AVPacket* pkt,
                   FILE* outfile) {
    int ret = 0;

    /* send the frame to the encoder */
    if (frame != nullptr) {
        fmt::println("Send frame {:3}", frame->pts);
}

    ret = avcodec_send_frame(encCtx, frame);
    if (ret < 0) {
        fmt::println( "Error sending a frame for encoding");
        exit(1);
    }

    while (true) {
        ret = avcodec_receive_packet(encCtx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } if (ret < 0) {
            fmt::println("Error during encoding");
            exit(1);
        }

        fmt::println("Write packet {:3} (size={:5})", pkt->pts, pkt->size);
        (void)fwrite(pkt->data, 1, pkt->size, outfile);
        av_packet_unref(pkt);
    }
}

int main(int argc, char** argv) { //NOLINT
    const char* infile = nullptr;
    const char* filename = nullptr;
    const char* codecName = nullptr;
    const AVCodec* codec = nullptr;
    AVCodecContext* c = nullptr;
    int x = 0;
    int y = 0;
    FILE* f = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* pkt = nullptr;
    std::array<unsigned int, 4> endcode = {0, 0, 1, 0xb7};

    if (argc <= 3) {
        fmt::println("Usage: {} <input file> <output file> <codec name>",
                argv[0]);
        exit(0);
    }
    infile = argv[1];
    filename = argv[2];
    codecName = argv[3];

    VisionLogReader vl(infile);

    /* find the mpeg1video encoder */
    codec = avcodec_find_encoder_by_name(codecName);
    if (codec == nullptr) {
        fmt::println("Codec '{}' not found", codecName);
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if (c == nullptr) {
        fmt::println("Could not allocate video codec context");
        exit(1);
    }

    pkt = av_packet_alloc();
    if (pkt == nullptr) {
        exit(1);
}

    /* put sample parameters */
    c->bit_rate = 6000000;
    /* resolution must be a multiple of two */
    c->width = 1280;
    c->height = 720;
    /* frames per second */
    c->time_base = AVRational{.num=1, .den=72};
    c->framerate = AVRational{.num=72, .den=1};

    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    c->gop_size = 10;
    c->max_b_frames = 1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;

    if (codec->id == AV_CODEC_ID_H264) {
        av_opt_set(c->priv_data, "preset", "slow", 0);
}

    /* open it */
    int ret = avcodec_open2(c, codec, nullptr);
    if (ret < 0) {
        std::array<char, AV_ERROR_MAX_STRING_SIZE> szErr{};
        av_make_error_string(szErr.data(), sizeof(szErr), ret);
        fmt::println("Could not open codec: {}", szErr.data());
        exit(1);
    }

    f = fopen(filename, "wb"); //NOLINT
    if (f == nullptr) {
        fmt::println("Could not open {}", filename);
        exit(1);
    }

    frame = av_frame_alloc();
    if (frame == nullptr) {
        fmt::println("Could not allocate video frame");
        exit(1);
    }
    frame->format = c->pix_fmt;
    frame->width = c->width;
    frame->height = c->height;

    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        fmt::println("Could not allocate the video frame data");
        exit(1);
    }

    /* encode 1 second of video */
    std::optional<cv::Mat> frm;
    while ((frm = vl.TryReadFrame())) {
        int i = 0;
        if (!frm) {
            break;
        }
        (void)fflush(stdout);

        /* Make sure the frame data is writable.
           On the first round, the frame is fresh from av_frame_get_buffer()
           and therefore we know it is writable.
           But on the next rounds, encode() will have called
           avcodec_send_frame(), and the codec may have kept a reference to
           the frame in its internal structures, that makes the frame
           unwritable.
           av_frame_make_writable() checks that and allocates a new buffer
           for the frame only if necessary.
         */
        ret = av_frame_make_writable(frame);
        if (ret < 0) {
            exit(1);
}

        /* Convert greyscal to YUV using
                |Y|   |  0.29900  0.58700  0.11400 | |R|
                |U| = | -0.14713 -0.28886  0.43600 | |G|
                |V|   |  0.61500 -0.51499 -0.10001 | |B|

                R=G=B = greyscale value

                Y = Gs
                U = 0.00027 Gs
                V = 0 Gs
        */

        /* Y */
        for (y = 0; y < c->height; y++) {
            for (x = 0; x < c->width; x++) {
                uint8_t u8I = frm->at<uint8_t>(y, x);
                frame->data[0][(y * frame->linesize[0]) + x] = frm->at<uint8_t>(y, x);
            }
        }

        /* Cb and Cr */
        for (y = 0; y < c->height / 2; y++) {
            for (x = 0; x < c->width / 2; x++) {
                frame->data[1][(y * frame->linesize[1]) + x] = 0;
                frame->data[2][(y * frame->linesize[2]) + x] = 0;
            }
        }

        frame->pts = i;

        /* encode the image */
        Encode(c, frame, pkt, f);
    }

    /* flush the encoder */
    Encode(c, nullptr, pkt, f);

    /* Add sequence end code to have a real MPEG file.
       It makes only sense because this tiny examples writes packets
       directly. This is called "elementary stream" and only works for some
       codecs. To create a valid file, you usually need to write packets
       into a proper file format or protocol; see mux.c.
     */
    if (codec->id == AV_CODEC_ID_MPEG1VIDEO ||
        codec->id == AV_CODEC_ID_MPEG2VIDEO) {
        (void)fwrite(endcode.data(), 1, sizeof(endcode), f);
}
    (void)fclose(f);//NOLINT

    avcodec_free_context(&c);
    av_frame_free(&frame);
    av_packet_free(&pkt);

    return 0;
}
