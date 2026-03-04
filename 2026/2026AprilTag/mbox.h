#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <iostream>

/**
 * @brief MailBox Template class
 *
 * The MailBox Template class provides an inter-thread queue of the specified message type.
 * The owner of the Mailbox can wait() for another process to post() messages.
 *   e.g. Mailbox<Msg *> is a mailbox accepting messages of type Msg *
 */
template <class T> class MailBox
{
	std::queue<T>			m_qMsgs;
	std::mutex				m_mtx;
	std::condition_variable	m_cv;
	bool					m_bShutdown = false;
public:
	/**
	 * The Default Constructor
	 * @brief constructor
	 */
	MailBox()
	{
		m_bShutdown = false;
	}
	/**
	 * The class destructor
	 * @brief destructor
	 */
	~MailBox()
	{
		shutdown();
	}

	/**
	 * @brief shutdowns down mailbox
	 *
	 * Shuts down the Mailbox.
	 * Threads that call wait() or waitfor() will return on the next
	 * wakeup regardless of whether or not there is a message pending.
	 */
	inline void shutdown()
	{
		if (!m_bShutdown) {
			std::unique_lock<std::mutex> lock(m_mtx);
			m_bShutdown = true;
			m_cv.notify_all();

			// Give all tasks a chance to wake up...
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}
	}

	/**
	 * @brief Post Message to mailbox
	 *
	 * Posts a message to the mailbox.
	 * The message is placed at the back of the mailbox queue.
	 * If there is a thread waiting, it will be awakend
	 * @param Msg [IN]
	 */
	inline void post( T &Msg )
	{
		std::unique_lock<std::mutex> lock(m_mtx);
		m_qMsgs.push( Msg );
		m_cv.notify_one();
	}

	/**
	 * @brief Wait for message indefinitely
	 *
	 * Waits indefinitely for a message to be posted to the mailbox.
	 * If there is at least one message in the queue, the first
	 * message will be removed from the queue and returned immediately.
	 * @param &msg [OUT]
	 * @return bool  true if message received, false if mailbox has been shut down
	 * @see waitfor()
	 */
	inline bool wait( T &msg )
	{
		std::unique_lock<std::mutex> lock(m_mtx);

		while (m_qMsgs.size() == 0 && !m_bShutdown) {
			m_cv.wait(lock);
		}

		bool bRet = false;
		if (!m_bShutdown) {
			msg = m_qMsgs.front();
			m_qMsgs.pop();
			bRet = true;
		}

		return bRet;
	}

	/**
	 * @brief Wait for message with timeout
	 *
	 * Waits for a message to be posted to the mailbox.
	 * If there is at least one message in the queue, the first
	 * message will be removed from the queue and the function will reutrn true.
	 * If a message has not been received within the specified time (in milliseconds),
	 * the msg parameter will not be modified, and the function will return false.
	 *
	 * @param msg  [OUT]
	 * @param u32Millis number of milliseconds to wait for message [IN]
	 * @return Message or null
	 * @see wait
	 */
	inline bool waitfor(T &msg, const uint32_t u32Millis)
	{
using namespace std::chrono;
		std::unique_lock<std::mutex> lock(m_mtx);
		std::chrono::time_point<steady_clock,nanoseconds> endTime;
		endTime = steady_clock::now() + milliseconds(u32Millis);
//		std::cout << "Wait dur = " << (duration_cast<duration<double>>(endTime - steady_clock::now())).count() << "\n";

		while (m_qMsgs.size() == 0 && !m_bShutdown && steady_clock::now() < endTime) {
			m_cv.wait_until(lock, endTime);
		}

		bool bRet = false;
		if (m_qMsgs.size()) {
			msg = m_qMsgs.front();
			m_qMsgs.pop();
			bRet = true;
		}

		return bRet;
	}
};
