//
// Created by 张腾 on 25-3-5.
//

#ifndef EXCEPTION_H
#define EXCEPTION_H
#include <exception>
#include <string>
#include <fmt/format.h>
class Exception: public std::exception
	/// This is the base class for all exceptions defined
	/// in the Poco class library.
{
public:
	explicit Exception(std::string  msg, int code = 0);
		/// Creates an exception.

	Exception(std::string  msg, const std::string& arg, int code = 0);
		/// Creates an exception.

	Exception(std::string  msg, const Exception& nested, int code = 0);
		/// Creates an exception and stores a clone
		/// of the nested exception.

	Exception(const Exception& exc);
		/// Copy constructor.

	~Exception() noexcept override;
		/// Destroys the exception and deletes the nested exception.

	Exception& operator = (const Exception& exc);
		/// Assignment operator.

	virtual const char* name() const noexcept;
		/// Returns a static string describing the exception.

	virtual const char* className() const noexcept;
		/// Returns the name of the exception class.

	const char* what() const noexcept override;
		/// Returns a static string describing the exception.
		///
		/// Same as name(), but for compatibility with std::exception.

	const Exception* nested() const;
		/// Returns a pointer to the nested exception, or
		/// null if no nested exception exists.

	const std::string& message() const;
		/// Returns the message text.

	int code() const;
		/// Returns the exception code if defined.

	std::string displayText() const;
		/// Returns a string consisting of the
		/// message name and the message text.

	virtual Exception* clone() const;
		/// Creates an exact copy of the exception.
		///
		/// The copy can later be thrown again by
		/// invoking rethrow() on it.

	virtual void rethrow() const;
		/// (Re)Throws the exception.
		///
		/// This is useful for temporarily storing a
		/// copy of an exception (see clone()), then
		/// throwing it again.

protected:
	explicit Exception(int code = 0);
		/// Standard constructor.

	void message(const std::string& msg);
		/// Sets the message for the exception.

	void extendedMessage(const std::string& arg);
		/// Sets the extended message for the exception.

private:
	std::string _msg;
	Exception*  _pNested;
	int			_code;
};

#if defined(_HAS_EXCEPTIONS)
// Size of Exception depends on the exception settings (like _HAS_EXCEPTIONS)
// that might influence size of std::exception from which Exception is derived from.
// It is expected that Poco libraries and application using Poco have the same settings.
static_assert(_HAS_EXCEPTIONS != 0);
#endif

//
// inlines
//
inline const Exception* Exception::nested() const
{
	return _pNested;
}


inline const std::string& Exception::message() const
{
	return _msg;
}


inline void Exception::message(const std::string& msg)
{
	_msg = msg;
}


inline int Exception::code() const
{
	return _code;
}


//
// Macros for quickly declaring and implementing exception classes.
// Unfortunately, we cannot use a template here because character
// pointers (which we need for specifying the exception name)
// are not allowed as template arguments.
//
#define DECLARE_EXCEPTION_CODE(CLS, BASE, CODE)														\
	class CLS: public BASE																			\
	{																								\
	public:																							\
		CLS(int code = CODE);																		\
		CLS(const std::string& msg, int code = CODE);												\
		CLS(const std::string& msg, const std::string& arg, int code = CODE);						\
		CLS(const std::string& msg, const Exception& exc, int code = CODE);							\
		CLS(const CLS& exc);																		\
		~CLS() noexcept;																			\
		CLS& operator = (const CLS& exc);															\
		const char* name() const noexcept;															\
		const char* className() const noexcept;														\
		Exception* clone() const;																	\
		void rethrow() const;																		\
	};

#define DECLARE_EXCEPTION(CLS, BASE)																\
	DECLARE_EXCEPTION_CODE(CLS, BASE, 0)

#define IMPLEMENT_EXCEPTION(CLS, BASE, NAME)														\
	CLS::CLS(int code): BASE(code)																	\
	{																								\
	}																								\
	CLS::CLS(const std::string& msg, int code): BASE(msg, code)										\
	{																								\
	}																								\
	CLS::CLS(const std::string& msg, const std::string& arg, int code): BASE(msg, arg, code)		\
	{																								\
	}																								\
	CLS::CLS(const std::string& msg, const Exception& exc, int code): BASE(msg, exc, code)			\
	{																								\
	}																								\
	CLS::CLS(const CLS& exc): BASE(exc)																\
	{																								\
	}																								\
	CLS::~CLS() noexcept																			\
	{																								\
	}																								\
	CLS& CLS::operator = (const CLS& exc)															\
	{																								\
		BASE::operator = (exc);																		\
		return *this;																				\
	}																								\
	const char* CLS::name() const noexcept															\
	{																								\
		return NAME;																				\
	}																								\
	const char* CLS::className() const noexcept														\
	{																								\
		return typeid(*this).name();																\
	}																								\
	Exception* CLS::clone() const																	\
	{																								\
		return new CLS(*this);																		\
	}																								\
	void CLS::rethrow() const																		\
	{																								\
		throw *this;																				\
	}


//
// Standard exception classes
//
DECLARE_EXCEPTION(LogicException, Exception)
DECLARE_EXCEPTION(AssertionViolationException, LogicException)
DECLARE_EXCEPTION(NullPointerException, LogicException)
DECLARE_EXCEPTION(NullValueException, LogicException)
DECLARE_EXCEPTION(BugcheckException, LogicException)
DECLARE_EXCEPTION(InvalidArgumentException, LogicException)
DECLARE_EXCEPTION(NotImplementedException, LogicException)
DECLARE_EXCEPTION(RangeException, LogicException)
DECLARE_EXCEPTION(IllegalStateException, LogicException)
DECLARE_EXCEPTION(InvalidAccessException, LogicException)
DECLARE_EXCEPTION(SignalException, LogicException)
DECLARE_EXCEPTION(UnhandledException, LogicException)
DECLARE_EXCEPTION(RuntimeException, Exception)
DECLARE_EXCEPTION(NotFoundException, RuntimeException)
DECLARE_EXCEPTION(ExistsException, RuntimeException)
DECLARE_EXCEPTION(TimeoutException, RuntimeException)
DECLARE_EXCEPTION(SystemException, RuntimeException)
DECLARE_EXCEPTION(RegularExpressionException, RuntimeException)
DECLARE_EXCEPTION(LibraryLoadException, RuntimeException)
DECLARE_EXCEPTION(LibraryAlreadyLoadedException, RuntimeException)
DECLARE_EXCEPTION(NoThreadAvailableException, RuntimeException)
DECLARE_EXCEPTION(PropertyNotSupportedException, RuntimeException)
DECLARE_EXCEPTION(PoolOverflowException, RuntimeException)
DECLARE_EXCEPTION(NoPermissionException, RuntimeException)
DECLARE_EXCEPTION(OutOfMemoryException, RuntimeException)
DECLARE_EXCEPTION(ResourceLimitException, RuntimeException)
DECLARE_EXCEPTION(DataException, RuntimeException)
DECLARE_EXCEPTION(DataFormatException, DataException)
DECLARE_EXCEPTION(SyntaxException, DataException)
DECLARE_EXCEPTION(CircularReferenceException, DataException)
DECLARE_EXCEPTION(PathSyntaxException, SyntaxException)
DECLARE_EXCEPTION(IOException, RuntimeException)
DECLARE_EXCEPTION(ProtocolException, IOException)
DECLARE_EXCEPTION(FileException, IOException)
DECLARE_EXCEPTION(FileExistsException, FileException)
DECLARE_EXCEPTION(FileNotFoundException, FileException)
DECLARE_EXCEPTION(PathNotFoundException, FileException)
DECLARE_EXCEPTION(FileReadOnlyException, FileException)
DECLARE_EXCEPTION(FileAccessDeniedException, FileException)
DECLARE_EXCEPTION(CreateFileException, FileException)
DECLARE_EXCEPTION(OpenFileException, FileException)
DECLARE_EXCEPTION(WriteFileException, FileException)
DECLARE_EXCEPTION(ReadFileException, FileException)
DECLARE_EXCEPTION(ExecuteFileException, FileException)
DECLARE_EXCEPTION(FileNotReadyException, FileException)
DECLARE_EXCEPTION(DirectoryNotEmptyException, FileException)
DECLARE_EXCEPTION(UnknownURISchemeException, RuntimeException)
DECLARE_EXCEPTION(TooManyURIRedirectsException, RuntimeException)
DECLARE_EXCEPTION(URISyntaxException, SyntaxException)
DECLARE_EXCEPTION(ApplicationException, Exception)
DECLARE_EXCEPTION(BadCastException, RuntimeException)
#endif //EXCEPTION_H
