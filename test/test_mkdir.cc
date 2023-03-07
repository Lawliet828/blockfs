#include <gtest/gtest.h>

#include "file_store_udisk.h"
#include "file_system.h"
#include "logging.h"

class BlockFsEnvironment : public testing::Environment {
private:
    const char *mount_point_ = "/mnt/mysql";
    const char *uuid_;
    bool master_ = true;

public:
    void SetUp() override {
        int ret = block_fs_mount("/root/blockfs/conf/bfs.cnf", master_);
        ASSERT_EQ(ret, 0);
        uuid_ = FileStore::Instance()->mount_config()->device_uuid_.c_str();
        // 设置测试文件中的日志等级
        Logger::set_min_level(LogLevel::FATAL);
    }
    void TearDown() override {
        int ret = block_fs_unmount(uuid_);
        EXPECT_EQ(ret, 0);
    }
};

class CreateDir : public ::testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(CreateDir, file_test) {

}

int main(int argc, char *argv[]) {
    testing::AddGlobalTestEnvironment(new BlockFsEnvironment());
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}