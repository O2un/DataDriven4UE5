export module ISingleton;

export namespace CommonHelper
{
	template <typename T>
	class Singleton
	{
	protected:
		Singleton() = default;
		virtual ~Singleton() = default;

	public:
		Singleton(const Singleton&) = delete;
		Singleton& operator=(const Singleton&) = delete;
		Singleton(Singleton&&) = delete;
		Singleton& operator=(Singleton&&) = delete;

	public:
		static T& Instance()
		{
			static T instance;
			return instance;
		}
	};
}