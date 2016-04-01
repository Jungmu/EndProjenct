#pragma once
//myo include
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <algorithm>

// The only file that needs to be included to use the Myo C++ SDK is myo.hpp.
#include <myo/myo.hpp>

// Myo 데이터를 받아서 가공해주는 클래스
class DataCollector : public myo::DeviceListener
{
public:
	
	// These values are set by onArmSync() and onArmUnsync() above.
	bool onArm;
	myo::Arm whichArm;

	// This is set by onUnlocked() and onLocked() above.
	bool isUnlocked;

	// These values are set by onOrientationData() and onPose() above.
	int roll_w, pitch_w, yaw_w;
	myo::Pose currentPose;

	DataCollector();
	~DataCollector();

	// onUnpair() is called whenever the Myo is disconnected from Myo Connect by the user.
	// 마이오 탐지가 안될때
	void onUnpair(myo::Myo* myo, uint64_t timestamp);

	// onOrientationData() is called whenever the Myo device provides its current orientation, which is represented
	// as a unit quaternion.
	// 마이오에서 받아온 Quaternion 데이터를 가공하여 각 축에대한 회전값으로 변환
	void onOrientationData(myo::Myo* myo, uint64_t timestamp, const myo::Quaternion<float>& quat);
	

	// onPose() is called whenever the Myo detects that the person wearing it has changed their pose, for example,
	// making a fist, or not making a fist anymore.
	// 마이오에서 사용자의 손동작을 읽어오는 함수
	void onPose(myo::Myo* myo, uint64_t timestamp, myo::Pose pose);

	// onArmSync() is called whenever Myo has recognized a Sync Gesture after someone has put it on their
	// arm. This lets Myo know which arm it's on and which way it's facing.
	// 마이오가 팔에 정상적으로 착용이 되었는지 탐지
	void onArmSync(myo::Myo* myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection, float rotation,myo::WarmupState warmupState);


	// onArmUnsync() is called whenever Myo has detected that it was moved from a stable position on a person's arm after
	// it recognized the arm. Typically this happens when someone takes Myo off of their arm, but it can also happen
	// when Myo is moved around on the arm.
	// 마이오를 팔에서 제거하였음을 탐지
	void onArmUnsync(myo::Myo* myo, uint64_t timestamp);

	// onUnlock() is called whenever Myo has become unlocked, and will start delivering pose events.
	void onUnlock(myo::Myo* myo, uint64_t timestamp);

	// onLock() is called whenever Myo has become locked. No pose events will be sent until the Myo is unlocked again.
	void onLock(myo::Myo* myo, uint64_t timestamp);

	// There are other virtual functions in DeviceListener that we could override here, like onAccelerometerData().
	// For this example, the functions overridden above are sufficient.

	// We define this function to print the current values that were updated by the on...() functions above.
	// 마이오의 상태를 콘솔을 통해 확인하기 위해 작성한 함수
	void print();
	
	
};

