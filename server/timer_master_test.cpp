// http://code.google.com/p/server1/
//
// You can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// Author: xiliu.tang@gmail.com (Xiliu Tang)

#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include "server/timer_master.hpp"
#include <sstream>

struct TestTimer : public Timer,
  public boost::enable_shared_from_this<TestTimer> {
  bool period() const {
    return period_;
  }
  bool timeout() const {
    return timeout_;
  }
  void Expired() {
    expired_ = true;
  }
  bool period_, expired_;
  int timeout_;
};


class TimerMasterTest : public testing::Test {
};

TEST_F(TimerMasterTest, Test1) {
  boost::shared_ptr<TestTimer> timer(new TestTimer);
  TimerMaster m;
  m.Register(timer);
  timer->period_ = true;
  timer->timeout_ = 1;
  timer->expired_ = false;
  for (int i = 1; i < 0xFFFF; ++i) {
    m.Update(i);
    ASSERT_TRUE(timer->expired_);
    timer->expired_ = false;
  }
  for (int i = 0xFFFF; i < 0xFFFFF; ++i) {
    m.Update(i);
    ASSERT_TRUE(timer->expired_);
    timer->expired_ = false;
  }
}

TEST_F(TimerMasterTest, Test2) {
  vector<boost::shared_ptr<TestTimer> > timers;
  TimerMaster m;
  for (int i = 1; i < 0x10000; ++i) {
    boost::shared_ptr<TestTimer> timer(new TestTimer);
    timer->period_ = false;
    timer->timeout_ = i;
    timer->expired_ = false;
    m.Register(timer);
    timers.push_back(timer);
  }
  for (int i = 1; i < 0x10000; ++i) {
    m.Update(i);
    ASSERT_TRUE(timers[i - 1]->expired_);
    timers[i - 1]->expired_ = false;
    VLOG(2) << i;
  }
  ASSERT_FALSE(timers[0]->expired_);
}

TEST_F(TimerMasterTest, Test3) {
  TimerMaster m;
  for (int i = 1; i < 0x10000; ++i) {
    boost::shared_ptr<TestTimer> timer(new TestTimer);
    timer->period_ = false;
    timer->timeout_ = i;
    timer->expired_ = false;
    m.Register(timer);
  }
  for (int i = 1; i < 0x10000; ++i) {
    m.Update(i);
  }
}

TEST_F(TimerMasterTest, Test4) {
  static const int magic[] = {
    1,
    2,
    5,
    255,
    254,
    256,
    0xFF00,
    0x1000,
    0xFF01,
    0xFFFF};
  int max_timeout = 0xFFFF;
  vector<boost::shared_ptr<TestTimer> > timers;
  TimerMaster m;
  for (int i = 0; i < arraysize(magic); ++i) {
    boost::shared_ptr<TestTimer> timer(new TestTimer);
    timer->period_ = true;
    timer->timeout_ = magic[i];
    timer->expired_ = false;
    m.Register(timer);
    timers.push_back(timer);
  }
  for (int i = 0; i < max_timeout; ++i) {
    m.Update(i);
    if (i > 0) {
      for (int j = 0; j < timers.size(); ++j) {
        if (i % timers[j]->timeout_ == 0) {
          ASSERT_TRUE(timers[j]->expired_);
          timers[j]->expired_ = false;
        }
      }
    }
  }
}

int main(int argc, char **argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
