#include "../includes/Timer.h"

Timer::Timer()
	: mSecondsPerCount(0.0), mDeltaTime(-1.0),
	mBaseTime(0), mPausedTime(0), mStopTime(0),
	mPrevTime(0), mCurrTime(0), mStopped(false)
{
	__int64 countsPerSec;
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&countsPerSec));
	mSecondsPerCount = 1.0 / static_cast<double>(countsPerSec);
}

float Timer::TotalTime() const
{
	if (mStopped)
	{
		return static_cast<float>(((mStopTime - mPausedTime) - mBaseTime) * mSecondsPerCount);
	}
	else
	{
		return static_cast<float>(((mCurrTime - mPausedTime) - mBaseTime) * mSecondsPerCount);
	}
}
float Timer::DeltaTime() const
{
	return static_cast<float>(mDeltaTime);
}

void Timer::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));

	mBaseTime = currTime;
	mPrevTime = currTime;
	mStopTime = 0;
	mStopped = false;
}

void Timer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&startTime));

	if (mStopped)
	{
		mPausedTime += (startTime - mStopTime);

		mPrevTime = startTime;
		mStopTime = 0;
		mStopped = false;
	}
}

void Timer::Stop()
{
	if (!mStopped)
	{
		__int64 currTime;
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));

		mStopTime = currTime;
		mStopped = true;
	}
}

void Timer::Tick()
{
	if (mStopped)
	{
		mDeltaTime = 0.0;
		return;
	}

	__int64 currTime;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));
	mCurrTime = currTime;

	mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;
	
	mPrevTime = currTime;

	if (mDeltaTime < 0.0)
	{
		mDeltaTime = 0.0;
	}
}