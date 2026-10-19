// System

#include <QtCore/QProcessEnvironment>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>

#include "cpu.h"

#ifdef _MSC_VER

// Architecture
#include <windows.h>

#endif

bool System::avx = true;

/*!
Get the Advanced Vector Extensions flag.
\param a Boolean, set to true to run AVX accelerated algorithms.
*/
bool System::Avx()
{
    return avx;
}

/*!
\brief Get the environment variable with Qt.
\param name Name.
*/
QString System::GetEnv(const QString& name)
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    return env.value(name);
}

/*!
\brief Get the resource file name given an ENV variable and the subfolder/name of the file.

Returns an empty string if the resource has not been found. Searches also in the current folder if the env variable is not defined.
\param env Name of the environment variable.
\param name Name of the file.
*/

QString System::GetResource(const QString& env, const QString& name)
{
    QString envvalue = System::GetEnv(env);
    if (envvalue.isEmpty())
    {
        // The env variable is not defined, we will try to find the file in the current directory
        envvalue = ".";
        if (name.isEmpty())
            return envvalue; // a precise filename has not been asked, we just fall back to the current directory

        QString fullPath = envvalue + name;
        QFileInfo check_file(fullPath);
        if (check_file.exists())
        {
            return fullPath;
        }
        else {  // if file is still not found, remove the path and search only the filename in the current directory
            QString justName = QFileInfo(name).fileName();
            QFileInfo check_file2(justName);
            if (check_file2.exists())
            {
                return justName;
            }

        }
    }
    else
    {
        return envvalue + name;
    }
    return QString();
}

/*
\brief Converts ms to minutes, seconds and miliseconds.
\param total Total miliseconds to convert.
*/
void System::MsToMinutes(int total, int& min, int& s, int& ms)
{
    // Milliseconds
    ms = total % 1000;

    // Seconds
    total /= 1000;
    s = total % 60;

    // Minutes
    total /= 60;
    min = total;
}

/*
\brief Create a string with the formatted elapsed time.
\param time Timer.
*/
QString System::Elapsed(const QElapsedTimer& time)
{
    // Elapsed time is in milliseconds
    int e = time.elapsed();
    int m, s, ms;
    MsToMinutes(e, m, s, ms);
    return QString("%1:%2:%3").arg(m).arg(s, 2, 10, QChar('0')).arg(ms, 3, 10, QChar('0'));
}

QString System::DateTime()
{
    // Date and time
    QDate date = QDate::currentDate();
    QTime time = QTime::currentTime();

    QString s = QString("%1%2%3-%4%5%6")
        .arg(date.year(), 4)
        .arg(date.month(), 2, 10, QChar('0'))
        .arg(date.day(), 2, 10, QChar('0'))
        .arg(time.hour(), 2, 10, QChar('0'))
        .arg(time.minute(), 2, 10, QChar('0'))
        .arg(time.second(), 2, 10, QChar('0'));
    return s;
}

/*!
\brief Translate bytes into human-readable format (Gib, Mib and Kib)
\param bytes Number of bytes
\param gib Output in Gib
\param mib Output in Mib
\param kib Output in Kib
*/
void System::BytesToGib(size_t bytes, size_t& gib, size_t& mib, size_t& kib)
{
    constexpr std::uint64_t GiB = 1024ull * 1024 * 1024;
    constexpr std::uint64_t MiB = 1024ull * 1024;
    constexpr std::uint64_t KiB = 1024ull;
    gib = bytes / GiB;
    bytes %= GiB;

    mib = bytes / MiB;
    bytes %= MiB;

    kib = bytes / KiB;
    bytes %= KiB;
}
