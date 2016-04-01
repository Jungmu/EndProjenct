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

// Myo �����͸� �޾Ƽ� �������ִ� Ŭ����
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
	// ���̿� Ž���� �ȵɶ�
	void onUnpair(myo::Myo* myo, uint64_t timestamp);

	// onOrientationData() is called whenever the Myo device provides its current orientation, which is represented
	// as a unit quaternion.
	// ���̿����� �޾ƿ� Quaternion �����͸� �����Ͽ� �� �࿡���� ȸ�������� ��ȯ
	void onOrientationData(myo::Myo* myo, uint64_t timestamp, const myo::Quaternion<float>& quat);
	

	// onPose() is called whenever the Myo detects that the person wearing it has changed their pose, for example,
	// making a fist, or not making a fist anymore.
	// ���̿����� ������� �յ����� �о���� �Լ�
	void onPose(myo::Myo* myo, uint64_t timestamp, myo::Pose pose);

	// onArmSync() is called whenever Myo has recognized a Sync Gesture after someone has put it on their
	// arm. This lets Myo know which arm it's on and which way it's facing.
	// ���̿��� �ȿ� ���������� ������ �Ǿ����� Ž��
	void onArmSync(myo::Myo* myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection, float rotation,myo::WarmupState warmupState);


	// onArmUnsync() is called whenever Myo has detected that it was moved from a stable position on a person's arm after
	// it recognized the arm. Typically this happens when someone takes Myo off of their arm, but it can also happen
	// when Myo is moved around on the arm.
	// ���̿��� �ȿ��� �����Ͽ����� Ž��
	void onArmUnsync(myo::Myo* myo, uint64_t timestamp);

	// onUnlock() is called whenever Myo has become unlocked, and will start delivering pose events.
	void onUnlock(myo::Myo* myo, uint64_t timestamp);

	// onLock() is called whenever Myo has become locked. No pose events will be sent until the Myo is unlocked again.
	void onLock(myo::Myo* myo, uint64_t timestamp);

	// There are other virtual functions in DeviceListener that we could override here, like onAccelerometerData().
	// For this example, the functions overridden above are sufficient.

	// We define this function to print the current values that were updated by the on...() functions above.
	// ���̿��� ���¸� �ܼ��� ���� Ȯ���ϱ� ���� �ۼ��� �Լ�
	void print();
	
	
};

