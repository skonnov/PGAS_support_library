#ifndef __DETAIL_H__
#define __DETAIL_H__

#include <cassert>
#include <complex>
#include <cstdint>
#include <type_traits>

#include <mpi.h>

template <class T>
MPI_Datatype create_mpi_type(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype types) {
    MPI_Datatype new_type;
    MPI_Type_struct(count, blocklens, indices, types, &new_type);
    MPI_Datatype resized_new_type;
    MPI_Type_create_resized(new_type,
                            // lower bound == min(indices) == indices[0]
                            indices[0],
                            (MPI_Aint)sizeof(T),
                            &resized_new_type);
    MPI_Type_commit(&resized_new_type);
    MPI_Type_free(&new_type);
    return resized_new_type;
}

template <class T>
MPI_Datatype get_mpi_type() noexcept
{
    MPI_Datatype mpi_type = MPI_DATATYPE_NULL;

    if (std::is_same<T, char>::value)
    {
        mpi_type = MPI_CHAR;
    }
    else if (std::is_same<T, signed char>::value)
    {
        mpi_type = MPI_SIGNED_CHAR;
    }
    else if (std::is_same<T, unsigned char>::value)
    {
        mpi_type = MPI_UNSIGNED_CHAR;
    }
    else if (std::is_same<T, wchar_t>::value)
    {
        mpi_type = MPI_WCHAR;
    }
    else if (std::is_same<T, signed short>::value)
    {
        mpi_type = MPI_SHORT;
    }
    else if (std::is_same<T, unsigned short>::value)
    {
        mpi_type = MPI_UNSIGNED_SHORT;
    }
    else if (std::is_same<T, signed int>::value)
    {
        mpi_type = MPI_INT;
    }
    else if (std::is_same<T, unsigned int>::value)
    {
        mpi_type = MPI_UNSIGNED;
    }
    else if (std::is_same<T, signed long int>::value)
    {
        mpi_type = MPI_LONG;
    }
    else if (std::is_same<T, unsigned long int>::value)
    {
        mpi_type = MPI_UNSIGNED_LONG;
    }
    else if (std::is_same<T, signed long long int>::value)
    {
        mpi_type = MPI_LONG_LONG;
    }
    else if (std::is_same<T, unsigned long long int>::value)
    {
        mpi_type = MPI_UNSIGNED_LONG_LONG;
    }
    else if (std::is_same<T, float>::value)
    {
        mpi_type = MPI_FLOAT;
    }
    else if (std::is_same<T, double>::value)
    {
        mpi_type = MPI_DOUBLE;
    }
    else if (std::is_same<T, long double>::value)
    {
        mpi_type = MPI_LONG_DOUBLE;
    }
    else if (std::is_same<T, std::int8_t>::value)
    {
        mpi_type = MPI_INT8_T;
    }
    else if (std::is_same<T, std::int16_t>::value)
    {
        mpi_type = MPI_INT16_T;
    }
    else if (std::is_same<T, std::int32_t>::value)
    {
        mpi_type = MPI_INT32_T;
    }
    else if (std::is_same<T, std::int64_t>::value)
    {
        mpi_type = MPI_INT64_T;
    }
    else if (std::is_same<T, std::uint8_t>::value)
    {
        mpi_type = MPI_UINT8_T;
    }
    else if (std::is_same<T, std::uint16_t>::value)
    {
        mpi_type = MPI_UINT16_T;
    }
    else if (std::is_same<T, std::uint32_t>::value)
    {
        mpi_type = MPI_UINT32_T;
    }
    else if (std::is_same<T, std::uint64_t>::value)
    {
        mpi_type = MPI_UINT64_T;
    }
    else if (std::is_same<T, bool>::value)
    {
        mpi_type = MPI_C_BOOL;
    }
    else if (std::is_same<T, std::complex<float>>::value)
    {
        mpi_type = MPI_C_COMPLEX;
    }
    else if (std::is_same<T, std::complex<double>>::value)
    {
        mpi_type = MPI_C_DOUBLE_COMPLEX;
    }
    else if (std::is_same<T, std::complex<long double>>::value)
    {
        mpi_type = MPI_C_LONG_DOUBLE_COMPLEX;
    }

    return mpi_type;
}

#endif // __DETAIL_H__
