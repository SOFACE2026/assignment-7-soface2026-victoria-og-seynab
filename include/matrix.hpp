#ifndef MATRIX_HPP
#define MATRIX_HPP

#include <ostream>
#include <iostream>
#include <vector>
#include <tuple>
#include <thread>
#include <exception>

template <typename T>
class Mat2D
{
public:
    Mat2D(size_t n_rows, size_t n_cols) : n_rows(n_rows), n_cols(n_cols), data(n_rows * n_cols)
    {
    }

    // multiply elements with a constant factor on the calling thread
    void multiply_single_threaded(T factor)
    {
        // Vi bruger en for-loop til at gennemgå hvert element i data-vektoren.
        // &val tager fat i selve tallet, så vi kan ændre det.
        for (auto &val : this->data){
            val = val * factor;
        }

    }

    // split the matrix in n parts and use multiple spawn one thread per paSrtition to perform the multiplication.
    void multiply_partitioned(T factor, size_t n_partitions)
    {
        // 1) split data into n partitions, for example if the number of elements is 100 and 4 partitions are used, each
        // partition will contain 25 elements.
        // We store the pairs of start and stop elements [(start_0, end_0), (start_1, end_1), ... (start_3, end_3)]
        std::vector<std::tuple<typename std::vector<T>::iterator, typename std::vector<T>::iterator>> slices;

        size_t elems_per_partition = this->data.size() / n_partitions;

        if (elems_per_partition == 0)
        {
            throw "Number of partitions exceed the number of elements";
        }

        size_t remainder = this->data.size() % elems_per_partition;

        for (size_t i = 0; i < n_partitions; i++)
        {
            auto start = this->data.begin() + (i * elems_per_partition);
            auto end = start + elems_per_partition;

            if (i == n_partitions - 1)
            {
                end += remainder;
            }

            slices.push_back(std::make_tuple(start, end));
        }

        // 2) execute the `multiply_slice` function using the intervals stored in `slices` as arguments
        // use std::get<0>(t) and std::get<1>(t) to get the `start` and `end` out of the tuple
        std::vector<std::thread> workers;
        for (auto &t : slices)
        {
            // spawn new thread for each partition, to carry out the `multiply_slice` function.
            //Der hentes start- og slut-iteratorer fra t
            auto start = std::get<0>(t);
            auto end = std::get<1>(t);

            //Starter ny tråd, hvor den indeholder funktionen, konstanten, start og slut
            workers.push_back(std::thread(multiply_slice, factor, start, end));
        }

        // 3) It is important that the `iterators` vector used witin the thread is not freed prematurely
        // one solution to this is having the calling thread join the worker threads
        // this ensures that the full computation is done when this function returns
        for (auto &worker : workers)
        {
            //Her samles alle threads.
            //Hvis tråden stadig kører, venter vi her
            if(worker.joinable()){
                worker.join(); 
                // join() fortæller tråden, den skal stoppe og vente indtil alle workers er færdige med deres udregninger.
            }
        }
    }

    void set(size_t row, size_t col, T value)
    {
        data[get_idx(row, col)] = value;
    }

    T get(size_t row, size_t col) const
    {
        return data[get_idx(row, col)];
    }

    size_t get_rows() const
    {
        return n_rows;
    }

    size_t get_cols() const
    {
        return n_cols;
    }

    // check if both arrays contain the same elements
    bool array_equal(Mat2D<T> &other)
    {
        if (other.n_rows != this->n_rows || other.n_cols != this->n_rows)
        {
            return false;
        }

        for (int i = 0; i < this->data.size(); ++i)
        {
            if (this->data[i] != other.data[i])
            {
                return false;
            }
        }

        return true;
    }

private:
    // 1-D vector layed out in a row-major order
    std::vector<T> data;
    size_t n_rows;
    size_t n_cols;

    // converts row and column index to an the correct index
    // of the contiguous memory block
    size_t get_idx(size_t row, size_t col) const
    {
        return (row * n_cols) + col;
    }

    // implmentation for multiplying a slice/interval of the data
    // this is a static method since it needs to be passed to a thread
    static void multiply_slice(T factor, typename std::vector<T>::iterator begin, typename std::vector<T>::iterator end)
    {
        for (typename std::vector<T>::iterator it = begin; it != end; ++it){
            *it = *it * factor;
        }
    }
};

// Implementation of the `<<` operator for the Mat2D class.
// This allows us to print the object in a human-readable format
template <typename T>
std::ostream &operator<<(std::ostream &os, const Mat2D<T> &m)
{
    size_t n_rows = m.get_rows();
    size_t n_cols = m.get_cols();
    for (size_t row = 0; row < n_rows; row++)
    {
        os << "|";
        for (size_t col = 0; col < n_cols; col++)
        {
            os << m.get(row, col);
            if (col != n_cols - 1)
            {
                os << "\t";
            }
        }
        os << "|";
        if (row != n_rows - 1)
        {
            os << std::endl;
        }
    }

    return os;
}

#endif