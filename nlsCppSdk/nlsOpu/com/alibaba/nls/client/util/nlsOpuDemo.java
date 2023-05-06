package com.alibaba.nls.client.util;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

class nlsOpuDemo {

    public void testOpuEncode(String inputFile, String outputFile) {

        FileInputStream fis = null;
        try {
            File file2 = new File(inputFile);
            fis = new FileInputStream(file2);
        } catch (Exception e) {
            e.printStackTrace();
        }

        FileOutputStream fopus = null;
        try {
            File file3 = new File(outputFile);
            file3.createNewFile();
            fopus = new FileOutputStream(file3);
        } catch (Exception e) {
            e.printStackTrace();
        }

        try {
            byte[] buffer = new byte[640];
            byte[] bytes = new byte[512];
            opuCoder coder = new opuCoder();
            int rate = 16000;
            long encoder = coder.createOpuEncoder(rate);

            while (fis.available() >= 640) {
                fis.read(buffer, 0, 640);
				//exg: the pcm data must be 640 byte
                int encodeSize = coder.opuEncoder(encoder, buffer, 640, bytes, 512);

                System.out.println("send len:" + encodeSize + " .");

				//exg: send opu data	
                fopus.write(bytes, 0, encodeSize);
            }
            fis.close();
            fopus.close();

            coder.destroyOpuEncoder(encoder);

        } catch (IOException e) {
            e.printStackTrace();
        }

    }

    public static void main(String[] args) throws IOException {

        final nlsOpuDemo opuTest = new nlsOpuDemo();

        opuTest.testOpuEncode(args[0], args[1]);
    }

}