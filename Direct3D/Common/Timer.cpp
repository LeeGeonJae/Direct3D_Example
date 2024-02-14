#include "pch.h"

#include "Timer.h"

namespace common
{
	Timer::Timer()
		: mSecondsPerCount(0.0)
		, mDeltaTime(0.0)
		, mBaseTime(0)
		, mAccumulatePausedTime(0)
		, mStopTime(0)
		, mPrevTime(0)
		, mCurrTime(0)
		, mbStopped(false)
	{
		__int64 countsPerSecond;
		QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&countsPerSecond));
		mSecondsPerCount = 1.0 / static_cast<double>(countsPerSecond);
	}

	void Timer::Reset()
	{
		__int64 currTime;
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));

		mBaseTime = currTime;
		mPrevTime = currTime;
		mStopTime = 0;
		mbStopped = false;
	}

	void Timer::Start()
	{
		if (!mbStopped)
		{
			return;
		}

		__int64 startTime;
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&startTime));

		mAccumulatePausedTime += (startTime - mStopTime);

		mPrevTime = startTime;
		mStopTime = 0;
		mbStopped = false;
	}

	void Timer::Pause()
	{
		if (mbStopped)
		{
			return;
		}

		__int64 currTime;
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));

		mStopTime = currTime;
		mbStopped = true;
	}

	void Timer::Update()
	{
		if (mbStopped)
		{
			mDeltaTime = 0.0;
			return;
		}

		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&mCurrTime));
		mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;
		mPrevTime = mCurrTime;

		if (mDeltaTime < 0.0)
		{
			mDeltaTime = 0.0;
		}
	}
}