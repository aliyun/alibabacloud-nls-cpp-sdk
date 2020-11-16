package com.alibaba.nls.client.util;

/**
 *
 * opu编解码器
 *
 * @author zhishen.ml
 * @Date 2018/12/7.
 */
public class OpuCodec {
    static {
        System.loadLibrary("opucodec");
    }

    public native long createOpuEncoder(int sampleRate);

    public native int encode(long encoder, byte[] frameBuff, int frameLen, byte[] outputBuffer, int outputLen);

    public native void destroyOpuEncoder(long handle);
}
