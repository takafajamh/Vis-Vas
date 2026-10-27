#pragma once

#include <memory>
#include <utility>
#include <vector>

template <typename DataT>
class DataFilter
{
public:
    virtual ~DataFilter() = default;
    virtual void Apply(std::vector<DataT>& data) = 0;
};

template <typename DataT>
class DataPipeline
{
public:
    DataPipeline() = default;

    explicit DataPipeline(std::vector<DataT> data)
        : m_data(std::move(data))
    {
    }

    void AddFilter(std::unique_ptr<DataFilter<DataT>> filter)
    {
        m_filters.push_back(std::move(filter));
    }

    void Apply()
    {
        for (const std::unique_ptr<DataFilter<DataT>>& filter : m_filters)
            filter->Apply(m_data);
    }

    const std::vector<DataT>& GetData() const { return m_data; }

private:
    std::vector<DataT> m_data;
    std::vector<std::unique_ptr<DataFilter<DataT>>> m_filters;
};
