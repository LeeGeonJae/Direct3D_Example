#pragma once

namespace common
{
	// 물방울책 : GameTime 참고

	class Timer
	{
	public:
		Timer();
		~Timer() = default;
		Timer(const Timer&) = default;
		Timer& operator=(const Timer&) = default;

		void Reset();
		void Start();
		void Pause();
		void Update();

		inline float GetTotalTime() const;
		inline float GetDeltaTime() const;

	private:
		double mSecondsPerCount;
		double mDeltaTime;

		__int64 mBaseTime;
		__int64 mStopTime;
		__int64 mAccumulatePausedTime;
		__int64 mPrevTime;
		__int64 mCurrTime;

		bool mbStopped;
	};

	float Timer::GetTotalTime() const
	{
		double result = mbStopped ?
			((mStopTime - mAccumulatePausedTime) - mBaseTime) * mSecondsPerCount :
			((mCurrTime - mAccumulatePausedTime) - mBaseTime) * mSecondsPerCount;

		return static_cast<float>(result);
	}

	float Timer::GetDeltaTime() const
	{
		return static_cast<float>(mDeltaTime);
	}
}