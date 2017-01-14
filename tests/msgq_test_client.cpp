/*
* Copyright (C) 2016 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <gtest/gtest.h>
#ifndef GTEST_IS_THREADSAFE
#error "GTest did not detect pthread library."
#endif

#include <fmq/MessageQueue.h>
#include <android/hardware/tests/msgq/1.0/ITestMsgQ.h>
#include <fmq/EventFlag.h>

// libutils:
using android::OK;
using android::sp;
using android::status_t;

// generated
using android::hardware::tests::msgq::V1_0::ITestMsgQ;

// libhidl
using android::hardware::kSynchronizedReadWrite;
using android::hardware::kUnsynchronizedWrite;
using android::hardware::MessageQueue;
using android::hardware::MQDescriptorSync;
using android::hardware::MQDescriptorUnsync;

namespace android {
namespace hardware {
namespace tests {
namespace client {

const char kServiceName[] = "android.hardware.tests.msgq@1.0::ITestMsgQ";

}  // namespace client
}  // namespace tests
}  // namespace hardware
}  // namespace android

class SynchronizedReadWriteClient : public ::testing::Test {
protected:
    virtual void TearDown() {
        delete mQueue;
    }

    virtual void SetUp() {
        namespace clientTests = android::hardware::tests::client;

        mService = ITestMsgQ::getService(clientTests::kServiceName);
        ASSERT_NE(mService, nullptr);
        mService->configureFmqSyncReadWrite([this](
                bool ret, const MQDescriptorSync<uint16_t>& in) {
            ASSERT_TRUE(ret);
            mQueue = new (std::nothrow) MessageQueue<uint16_t, kSynchronizedReadWrite>(in);
        });
        ASSERT_NE(nullptr, mQueue);
        ASSERT_TRUE(mQueue->isValid());
        mNumMessagesMax = mQueue->getQuantumCount();
    }

    sp<ITestMsgQ> mService;
    MessageQueue<uint16_t, kSynchronizedReadWrite>* mQueue = nullptr;
    size_t mNumMessagesMax = 0;
};

class UnsynchronizedWriteClient : public ::testing::Test {
protected:
  virtual void TearDown() {
      delete mQueue;
  }

  virtual void SetUp() {
      namespace clientTests = android::hardware::tests::client;

      mService = ITestMsgQ::getService(clientTests::kServiceName);
      ASSERT_NE(mService, nullptr);
      mService->configureFmqUnsyncWrite(
              [this](bool ret, const MQDescriptorUnsync<uint16_t>& in) {
                  ASSERT_TRUE(ret);
                  mQueue = new (std::nothrow) MessageQueue<uint16_t, kUnsynchronizedWrite>(in);
              });
      ASSERT_NE(nullptr, mQueue);
      ASSERT_TRUE(mQueue->isValid());
      mNumMessagesMax = mQueue->getQuantumCount();
  }

  sp<ITestMsgQ> mService;
  MessageQueue<uint16_t, kUnsynchronizedWrite>* mQueue = nullptr;
  size_t mNumMessagesMax = 0;
};

/*
 * Utility function to verify data read from the fast message queue.
 */
bool verifyData(uint16_t* data, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (data[i] != i) return false;
    }
    return true;
}

/*
 * Test that basic blocking works using readBlocking()/writeBlocking() APIs
 * using the EventFlag object owned by FMQ.
 */
TEST_F(SynchronizedReadWriteClient, BlockingReadWrite1) {
    const size_t dataLen = 64;
    uint16_t data[dataLen] = {0};

    /*
     * Request service to perform a blocking read. This call is oneway and will
     * return immediately.
     */
    mService->requestBlockingRead(dataLen);
    bool ret = mQueue->writeBlocking(data,
                                     dataLen,
                                     static_cast<uint32_t>(ITestMsgQ::EventFlagBits::FMQ_NOT_FULL),
                                     static_cast<uint32_t>(ITestMsgQ::EventFlagBits::FMQ_NOT_EMPTY),
                                     5000000000 /* timeOutNanos */);
    ASSERT_TRUE(ret);
}

/*
 * Test that basic blocking works using readBlocking()/writeBlocking() APIs
 * using the EventFlag object owned by FMQ and using the default EventFlag
 * notification bit mask.
 */
TEST_F(SynchronizedReadWriteClient, BlockingReadWrite2) {
    const size_t dataLen = 64;
    uint16_t data[dataLen] = {0};

    /*
     * Request service to perform a blocking read using default EventFlag
     * notification bit mask. This call is oneway and will
     * return immediately.
     */
    mService->requestBlockingReadDefaultEventFlagBits(dataLen);
    bool ret = mQueue->writeBlocking(data,
                                     dataLen,
                                     5000000000 /* timeOutNanos */);
    ASSERT_TRUE(ret);
}

/*
 * Test that repeated blocking reads and writes work using readBlocking()/writeBlocking() APIs
 * using the EventFlag object owned by FMQ.
 * Each write operation writes the same amount of data as a single read
 * operation.
 */
TEST_F(SynchronizedReadWriteClient, BlockingReadWriteRepeat1) {
    const size_t dataLen = 64;
    uint16_t data[dataLen] = {0};

    /*
     * Request service to perform a blocking read. This call is oneway and will
     * return immediately.
     */
    const size_t writeCount = 1024;
    mService->requestBlockingReadRepeat(dataLen, writeCount);

    for(size_t i = 0; i < writeCount; i++) {
        bool ret = mQueue->writeBlocking(
                data,
                dataLen,
                static_cast<uint32_t>(ITestMsgQ::EventFlagBits::FMQ_NOT_FULL),
                static_cast<uint32_t>(ITestMsgQ::EventFlagBits::FMQ_NOT_EMPTY),
                5000000000 /* timeOutNanos */);
        ASSERT_TRUE(ret);
    }
}

/*
 * Test that repeated blocking reads and writes work using readBlocking()/writeBlocking() APIs
 * using the EventFlag object owned by FMQ. Each read operation reads twice the
 * amount of data as a single write.
 *
 */
TEST_F(SynchronizedReadWriteClient, BlockingReadWriteRepeat2) {
    const size_t dataLen = 64;
    uint16_t data[dataLen] = {0};

    /*
     * Request service to perform a blocking read. This call is oneway and will
     * return immediately.
     */
    const size_t writeCount = 1024;
    mService->requestBlockingReadRepeat(dataLen*2, writeCount/2);

    for(size_t i = 0; i < writeCount; i++) {
        bool ret = mQueue->writeBlocking(
                data,
                dataLen,
                static_cast<uint32_t>(ITestMsgQ::EventFlagBits::FMQ_NOT_FULL),
                static_cast<uint32_t>(ITestMsgQ::EventFlagBits::FMQ_NOT_EMPTY),
                5000000000 /* timeOutNanos */);
        ASSERT_TRUE(ret);
    }
}

/*
 * Test that basic blocking works using readBlocking()/writeBlocking() APIs
 * using the EventFlag object owned by FMQ. Each write operation writes twice
 * the amount of data as a single read.
 */
TEST_F(SynchronizedReadWriteClient, BlockingReadWriteRepeat3) {
    const size_t dataLen = 64;
    uint16_t data[dataLen] = {0};

    /*
     * Request service to perform a blocking read. This call is oneway and will
     * return immediately.
     */
    size_t writeCount = 1024;
    mService->requestBlockingReadRepeat(dataLen/2, writeCount*2);

    for(size_t i = 0; i < writeCount; i++) {
        bool ret = mQueue->writeBlocking(
                data,
                dataLen,
                static_cast<uint32_t>(ITestMsgQ::EventFlagBits::FMQ_NOT_FULL),
                static_cast<uint32_t>(ITestMsgQ::EventFlagBits::FMQ_NOT_EMPTY),
                5000000000 /* timeOutNanos */);
        ASSERT_TRUE(ret);
    }
}

/*
 * Test that writeBlocking()/readBlocking() APIs do not block on
 * attempts to write/read 0 messages and return true.
 */
TEST_F(SynchronizedReadWriteClient, BlockingReadWriteZeroMessages) {
    uint16_t data = 0;

    /*
     * Trigger a blocking write for zero messages with no timeout.
     */
    bool ret = mQueue->writeBlocking(
            &data,
            0,
            static_cast<uint32_t>(ITestMsgQ::EventFlagBits::FMQ_NOT_FULL),
            static_cast<uint32_t>(ITestMsgQ::EventFlagBits::FMQ_NOT_EMPTY));
    ASSERT_TRUE(ret);

    /*
     * Trigger a blocking read for zero messages with no timeout.
     */
    ret = mQueue->readBlocking(&data,
                               0,
                               static_cast<uint32_t>(ITestMsgQ::EventFlagBits::FMQ_NOT_FULL),
                               static_cast<uint32_t>(ITestMsgQ::EventFlagBits::FMQ_NOT_EMPTY));
    ASSERT_TRUE(ret);
}

/* Request mService to write a small number of messages
 * to the FMQ. Read and verify data.
 */
TEST_F(SynchronizedReadWriteClient, SmallInputReaderTest1) {
    const size_t dataLen = 16;
    ASSERT_LE(dataLen, mNumMessagesMax);
    bool ret = mService->requestWriteFmqSync(dataLen);
    ASSERT_TRUE(ret);
    uint16_t readData[dataLen] = {};
    ASSERT_TRUE(mQueue->read(readData, dataLen));
    ASSERT_TRUE(verifyData(readData, dataLen));
}

/*
 * Write a small number of messages to FMQ. Request
 * mService to read and verify that the write was succesful.
 */
TEST_F(SynchronizedReadWriteClient, SmallInputWriterTest1) {
    const size_t dataLen = 16;
    ASSERT_LE(dataLen, mNumMessagesMax);
    size_t originalCount = mQueue->availableToWrite();
    uint16_t data[dataLen];
    for (size_t i = 0; i < dataLen; i++) {
        data[i] = i;
    }
    ASSERT_TRUE(mQueue->write(data, dataLen));
    bool ret = mService->requestReadFmqSync(dataLen);
    ASSERT_TRUE(ret);
    size_t availableCount = mQueue->availableToWrite();
    ASSERT_EQ(originalCount, availableCount);
}

/*
 * Verify that the FMQ is empty and read fails when it is empty.
 */
TEST_F(SynchronizedReadWriteClient, ReadWhenEmpty) {
    ASSERT_EQ(0UL, mQueue->availableToRead());
    const size_t numMessages = 2;
    ASSERT_LE(numMessages, mNumMessagesMax);
    uint16_t readData[numMessages];
    ASSERT_FALSE(mQueue->read(readData, numMessages));
}

/*
 * Verify FMQ is empty.
 * Write enough messages to fill it.
 * Verify availableToWrite() method returns is zero.
 * Try writing another message and verify that
 * the attempted write was unsuccesful. Request mService
 * to read and verify the messages in the FMQ.
 */

TEST_F(SynchronizedReadWriteClient, WriteWhenFull) {
    std::vector<uint16_t> data(mNumMessagesMax);
    for (size_t i = 0; i < mNumMessagesMax; i++) {
        data[i] = i;
    }
    ASSERT_TRUE(mQueue->write(&data[0], mNumMessagesMax));
    ASSERT_EQ(0UL, mQueue->availableToWrite());
    ASSERT_FALSE(mQueue->write(&data[0], 1));
    bool ret = mService->requestReadFmqSync(mNumMessagesMax);
    ASSERT_TRUE(ret);
}

/*
 * Verify FMQ is empty.
 * Request mService to write data equal to queue size.
 * Read and verify data in mQueue.
 */
TEST_F(SynchronizedReadWriteClient, LargeInputTest1) {
    bool ret = mService->requestWriteFmqSync(mNumMessagesMax);
    ASSERT_TRUE(ret);
    std::vector<uint16_t> readData(mNumMessagesMax);
    ASSERT_TRUE(mQueue->read(&readData[0], mNumMessagesMax));
    ASSERT_TRUE(verifyData(&readData[0], mNumMessagesMax));
}

/*
 * Request mService to write more than maximum number of messages to the FMQ.
 * Verify that the write fails. Verify that availableToRead() method
 * still returns 0 and verify that attempt to read fails.
 */
TEST_F(SynchronizedReadWriteClient, LargeInputTest2) {
    ASSERT_EQ(0UL, mQueue->availableToRead());
    const size_t numMessages = 2048;
    ASSERT_GT(numMessages, mNumMessagesMax);
    bool ret = mService->requestWriteFmqSync(numMessages);
    ASSERT_FALSE(ret);
    uint16_t readData;
    ASSERT_EQ(0UL, mQueue->availableToRead());
    ASSERT_FALSE(mQueue->read(&readData, 1));
}

/*
 * Write until FMQ is full.
 * Verify that the number of messages available to write
 * is equal to mNumMessagesMax.
 * Verify that another write attempt fails.
 * Request mService to read. Verify read count.
 */

TEST_F(SynchronizedReadWriteClient, LargeInputTest3) {
    std::vector<uint16_t> data(mNumMessagesMax);
    for (size_t i = 0; i < mNumMessagesMax; i++) {
        data[i] = i;
    }

    ASSERT_TRUE(mQueue->write(&data[0], mNumMessagesMax));
    ASSERT_EQ(0UL, mQueue->availableToWrite());
    ASSERT_FALSE(mQueue->write(&data[0], 1));

    bool ret = mService->requestReadFmqSync(mNumMessagesMax);
    ASSERT_TRUE(ret);
}

/*
 * Confirm that the FMQ is empty. Request mService to write to FMQ.
 * Do multiple reads to empty FMQ and verify data.
 */
TEST_F(SynchronizedReadWriteClient, MultipleRead) {
    const size_t chunkSize = 100;
    const size_t chunkNum = 5;
    const size_t numMessages = chunkSize * chunkNum;
    ASSERT_LE(numMessages, mNumMessagesMax);
    size_t availableToRead = mQueue->availableToRead();
    size_t expectedCount = 0;
    ASSERT_EQ(expectedCount, availableToRead);
    bool ret = mService->requestWriteFmqSync(numMessages);
    ASSERT_TRUE(ret);
    uint16_t readData[numMessages] = {};
    for (size_t i = 0; i < chunkNum; i++) {
        ASSERT_TRUE(mQueue->read(readData + i * chunkSize, chunkSize));
    }
    ASSERT_TRUE(verifyData(readData, numMessages));
}

/*
 * Write to FMQ in bursts.
 * Request mService to read data. Verify the read was successful.
 */
TEST_F(SynchronizedReadWriteClient, MultipleWrite) {
    const size_t chunkSize = 100;
    const size_t chunkNum = 5;
    const size_t numMessages = chunkSize * chunkNum;
    ASSERT_LE(numMessages, mNumMessagesMax);
    uint16_t data[numMessages];
    for (size_t i = 0; i < numMessages; i++) {
        data[i] = i;
    }
    for (size_t i = 0; i < chunkNum; i++) {
        ASSERT_TRUE(mQueue->write(data + i * chunkSize, chunkSize));
    }
    bool ret = mService->requestReadFmqSync(numMessages);
    ASSERT_TRUE(ret);
}

/*
 * Write enough messages into the FMQ to fill half of it.
 * Request mService to read back the same.
 * Write mNumMessagesMax messages into the queue. This should cause a
 * wrap around. Request mService to read and verify the data.
 */
TEST_F(SynchronizedReadWriteClient, ReadWriteWrapAround) {
    size_t numMessages = mNumMessagesMax / 2;
    std::vector<uint16_t> data(mNumMessagesMax);
    for (size_t i = 0; i < mNumMessagesMax; i++) {
        data[i] = i;
    }
    ASSERT_TRUE(mQueue->write(&data[0], numMessages));
    bool ret = mService->requestReadFmqSync(numMessages);
    ASSERT_TRUE(ret);
    ASSERT_TRUE(mQueue->write(&data[0], mNumMessagesMax));
    ret = mService->requestReadFmqSync(mNumMessagesMax);
    ASSERT_TRUE(ret);
}

/* Request mService to write a small number of messages
 * to the FMQ. Read and verify data.
 */
TEST_F(UnsynchronizedWriteClient, SmallInputReaderTest1) {
    const size_t dataLen = 16;
    ASSERT_LE(dataLen, mNumMessagesMax);
    bool ret = mService->requestWriteFmqUnsync(dataLen);
    ASSERT_TRUE(ret);
    uint16_t readData[dataLen] = {};
    ASSERT_TRUE(mQueue->read(readData, dataLen));
    ASSERT_TRUE(verifyData(readData, dataLen));
}

/*
 * Write a small number of messages to FMQ. Request
 * mService to read and verify that the write was succesful.
 */
TEST_F(UnsynchronizedWriteClient, SmallInputWriterTest1) {
    const size_t dataLen = 16;
    ASSERT_LE(dataLen, mNumMessagesMax);
    size_t originalCount = mQueue->availableToWrite();
    uint16_t data[dataLen];
    for (size_t i = 0; i < dataLen; i++) {
        data[i] = i;
    }
    ASSERT_TRUE(mQueue->write(data, dataLen));
    bool ret = mService->requestReadFmqUnsync(dataLen);
    ASSERT_TRUE(ret);
}

/*
 * Verify that the FMQ is empty and read fails when it is empty.
 */
TEST_F(UnsynchronizedWriteClient, ReadWhenEmpty) {
    ASSERT_EQ(0UL, mQueue->availableToRead());
    const size_t numMessages = 2;
    ASSERT_LE(numMessages, mNumMessagesMax);
    uint16_t readData[numMessages];
    ASSERT_FALSE(mQueue->read(readData, numMessages));
}

/*
 * Verify FMQ is empty.
 * Write enough messages to fill it.
 * Verify availableToWrite() method returns is zero.
 * Try writing another message and verify that
 * the attempted write was successful. Request mService
 * to read the messages in the FMQ and verify that it is unsuccesful.
 */

TEST_F(UnsynchronizedWriteClient, WriteWhenFull) {
    std::vector<uint16_t> data(mNumMessagesMax);
    for (size_t i = 0; i < mNumMessagesMax; i++) {
        data[i] = i;
    }
    ASSERT_TRUE(mQueue->write(&data[0], mNumMessagesMax));
    ASSERT_EQ(0UL, mQueue->availableToWrite());
    ASSERT_TRUE(mQueue->write(&data[0], 1));
    bool ret = mService->requestReadFmqUnsync(mNumMessagesMax);
    ASSERT_FALSE(ret);
}

/*
 * Verify FMQ is empty.
 * Request mService to write data equal to queue size.
 * Read and verify data in mQueue.
 */
TEST_F(UnsynchronizedWriteClient, LargeInputTest1) {
    bool ret = mService->requestWriteFmqUnsync(mNumMessagesMax);
    ASSERT_TRUE(ret);
    std::vector<uint16_t> data(mNumMessagesMax);
    ASSERT_TRUE(mQueue->read(&data[0], mNumMessagesMax));
    ASSERT_TRUE(verifyData(&data[0], mNumMessagesMax));
}

/*
 * Request mService to write more than maximum number of messages to the FMQ.
 * Verify that the write fails. Verify that availableToRead() method
 * still returns 0 and verify that attempt to read fails.
 */
TEST_F(UnsynchronizedWriteClient, LargeInputTest2) {
    ASSERT_EQ(0UL, mQueue->availableToRead());
    const size_t numMessages = mNumMessagesMax + 1;
    bool ret = mService->requestWriteFmqUnsync(numMessages);
    ASSERT_FALSE(ret);
    uint16_t readData;
    ASSERT_EQ(0UL, mQueue->availableToRead());
    ASSERT_FALSE(mQueue->read(&readData, 1));
}

/*
 * Write until FMQ is full.
 * Verify that the number of messages available to write
 * is equal to mNumMessagesMax.
 * Verify that another write attempt is succesful.
 * Request mService to read. Verify that read is unsuccessful.
 * Perform another write and verify that the read is succesful
 * to check if the reader process can recover from the error condition.
 */
TEST_F(UnsynchronizedWriteClient, LargeInputTest3) {
    std::vector<uint16_t> data(mNumMessagesMax);
    for (size_t i = 0; i < mNumMessagesMax; i++) {
        data[i] = i;
    }

    ASSERT_TRUE(mQueue->write(&data[0], mNumMessagesMax));
    ASSERT_EQ(0UL, mQueue->availableToWrite());
    ASSERT_TRUE(mQueue->write(&data[0], 1));

    bool ret = mService->requestReadFmqUnsync(mNumMessagesMax);
    ASSERT_FALSE(ret);
    ASSERT_TRUE(mQueue->write(&data[0], mNumMessagesMax));

    ret = mService->requestReadFmqUnsync(mNumMessagesMax);
    ASSERT_TRUE(ret);
}

/*
 * Confirm that the FMQ is empty. Request mService to write to FMQ.
 * Do multiple reads to empty FMQ and verify data.
 */
TEST_F(UnsynchronizedWriteClient, MultipleRead) {
    const size_t chunkSize = 100;
    const size_t chunkNum = 5;
    const size_t numMessages = chunkSize * chunkNum;
    ASSERT_LE(numMessages, mNumMessagesMax);
    size_t availableToRead = mQueue->availableToRead();
    size_t expectedCount = 0;
    ASSERT_EQ(expectedCount, availableToRead);
    bool ret = mService->requestWriteFmqUnsync(numMessages);
    ASSERT_TRUE(ret);
    uint16_t readData[numMessages] = {};
    for (size_t i = 0; i < chunkNum; i++) {
        ASSERT_TRUE(mQueue->read(readData + i * chunkSize, chunkSize));
    }
    ASSERT_TRUE(verifyData(readData, numMessages));
}

/*
 * Write to FMQ in bursts.
 * Request mService to read data, verify that it was successful.
 */
TEST_F(UnsynchronizedWriteClient, MultipleWrite) {
    const size_t chunkSize = 100;
    const size_t chunkNum = 5;
    const size_t numMessages = chunkSize * chunkNum;
    ASSERT_LE(numMessages, mNumMessagesMax);
    uint16_t data[numMessages];
    for (size_t i = 0; i < numMessages; i++) {
        data[i] = i;
    }
    for (size_t i = 0; i < chunkNum; i++) {
        ASSERT_TRUE(mQueue->write(data + i * chunkSize, chunkSize));
    }
    bool ret = mService->requestReadFmqUnsync(numMessages);
    ASSERT_TRUE(ret);
}

/*
 * Write enough messages into the FMQ to fill half of it.
 * Request mService to read back the same.
 * Write mNumMessagesMax messages into the queue. This should cause a
 * wrap around. Request mService to read and verify the data.
 */
TEST_F(UnsynchronizedWriteClient, ReadWriteWrapAround) {
    size_t numMessages = mNumMessagesMax / 2;
    std::vector<uint16_t> data(mNumMessagesMax);
    for (size_t i = 0; i < mNumMessagesMax; i++) {
        data[i] = i;
    }
    ASSERT_TRUE(mQueue->write(&data[0], numMessages));
    bool ret = mService->requestReadFmqUnsync(numMessages);
    ASSERT_TRUE(ret);
    ASSERT_TRUE(mQueue->write(&data[0], mNumMessagesMax));
    ret = mService->requestReadFmqUnsync(mNumMessagesMax);
    ASSERT_TRUE(ret);
}

/*
 * Request mService to write a small number of messages
 * to the FMQ. Read and verify data from two threads configured
 * as readers to the FMQ.
 */
TEST_F(UnsynchronizedWriteClient, SmallInputMultipleReaderTest) {
    auto desc = mQueue->getDesc();
    std::unique_ptr<MessageQueue<uint16_t, kUnsynchronizedWrite>> mQueue2(
            new (std::nothrow) MessageQueue<uint16_t, kUnsynchronizedWrite>(*desc));
    ASSERT_NE(nullptr, mQueue2.get());

    const size_t dataLen = 16;
    ASSERT_LE(dataLen, mNumMessagesMax);

    bool ret = mService->requestWriteFmqUnsync(dataLen);
    ASSERT_TRUE(ret);

    pid_t pid;
    if ((pid = fork()) == 0) {
        /* child process */
        uint16_t readData[dataLen] = {};
        ASSERT_TRUE(mQueue2->read(readData, dataLen));
        ASSERT_TRUE(verifyData(readData, dataLen));
        exit(0);
    } else {
        ASSERT_GT(pid,
                  0 /* parent should see PID greater than 0 for a good fork */);
        uint16_t readData[dataLen] = {};
        ASSERT_TRUE(mQueue->read(readData, dataLen));
        ASSERT_TRUE(verifyData(readData, dataLen));
    }
}

/*
 * Request mService to write into the FMQ until it is full.
 * Request mService to do another write and verify it is successful.
 * Use two reader threads to read and verify that both fail.
 */
TEST_F(UnsynchronizedWriteClient, MultipleReadersAfterOverflow1) {
    auto desc = mQueue->getDesc();
    std::unique_ptr<MessageQueue<uint16_t, kUnsynchronizedWrite>> mQueue2(
            new (std::nothrow) MessageQueue<uint16_t, kUnsynchronizedWrite>(*desc));
    ASSERT_NE(nullptr, mQueue2.get());

    bool ret = mService->requestWriteFmqUnsync(mNumMessagesMax);
    ASSERT_TRUE(ret);
    ret = mService->requestWriteFmqUnsync(1);
    ASSERT_TRUE(ret);

    pid_t pid;
    if ((pid = fork()) == 0) {
        /* child process */
        std::vector<uint16_t> readData(mNumMessagesMax);
        ASSERT_FALSE(mQueue2->read(&readData[0], mNumMessagesMax));
        exit(0);
    } else {
        ASSERT_GT(pid, 0/* parent should see PID greater than 0 for a good fork */);
        std::vector<uint16_t> readData(mNumMessagesMax);
        ASSERT_FALSE(mQueue->read(&readData[0], mNumMessagesMax));
    }
}

/*
 * Request mService to write into the FMQ until it is full.
 * Request mService to do another write and verify it is successful.
 * Use two reader threads to read and verify that both fail.
 * Request mService to write more data into the Queue and verify that both
 * readers are able to recover from the overflow and read successfully.
 */
TEST_F(UnsynchronizedWriteClient, MultipleReadersAfterOverflow2) {
    auto desc = mQueue->getDesc();
    std::unique_ptr<MessageQueue<uint16_t, kUnsynchronizedWrite>> mQueue2(
            new (std::nothrow) MessageQueue<uint16_t, kUnsynchronizedWrite>(*desc));
    ASSERT_NE(nullptr, mQueue2.get());

    bool ret = mService->requestWriteFmqUnsync(mNumMessagesMax);
    ASSERT_TRUE(ret);
    ret = mService->requestWriteFmqUnsync(1);
    ASSERT_TRUE(ret);

    const size_t dataLen = 16;
    ASSERT_LT(dataLen, mNumMessagesMax);

    pid_t pid;
    if ((pid = fork()) == 0) {
        /* child process */
        std::vector<uint16_t> readData(mNumMessagesMax);
        ASSERT_FALSE(mQueue2->read(&readData[0], mNumMessagesMax));
        ret = mService->requestWriteFmqUnsync(dataLen);
        ASSERT_TRUE(ret);
        ASSERT_TRUE(mQueue2->read(&readData[0], dataLen));
        ASSERT_TRUE(verifyData(&readData[0], dataLen));
        exit(0);
    } else {
        ASSERT_GT(pid, 0/* parent should see PID greater than 0 for a good fork */);

        std::vector<uint16_t> readData(mNumMessagesMax);
        ASSERT_FALSE(mQueue->read(&readData[0], mNumMessagesMax));

        int status;
        /*
         * Wait for child process to return before proceeding since the final write
         * is requested by the child.
         */
        ASSERT_EQ(pid, waitpid(pid, &status, 0 /* options */));

        ASSERT_TRUE(mQueue->read(&readData[0], dataLen));
        ASSERT_TRUE(verifyData(&readData[0], dataLen));
    }
}
