/***************************************************************************                                                                                     
*   Copyright 2012 - 2013 Advanced Micro Devices, Inc.                                     
*                                                                                    
*   Licensed under the Apache License, Version 2.0 (the "License");   
*   you may not use this file except in compliance with the License.                 
*   You may obtain a copy of the License at                                          
*                                                                                    
*       http://www.apache.org/licenses/LICENSE-2.0                      
*                                                                                    
*   Unless required by applicable law or agreed to in writing, software              
*   distributed under the License is distributed on an "AS IS" BASIS,              
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.         
*   See the License for the specific language governing permissions and              
*   limitations under the License.                                                   

***************************************************************************/

#define ENABLE_GTEST 1
#define ENABLE_DEBUGGING 0
#define UDD 1
#define TEST_DOUBLE 1
//#include "common/stdafx.h"
#include <vector>
//#include <array>
#include "bolt/cl/bolt.h"
#include "bolt/cl/functional.h"
#include "bolt/cl/reduce_by_key.h"
#include "bolt/unicode.h"
#include "bolt/miniDump.h"

#include <gtest/gtest.h>
//#include <boost/shared_array.hpp>

#include <boost/program_options.hpp>
#include "test_common.h"

#if !ENABLE_GTEST

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator1,
    typename OutputIterator2,
    typename BinaryFunction>
std::pair<OutputIterator1, OutputIterator2>
gold_reduce_by_key(
    InputIterator1 keys_first,
    InputIterator1 keys_last,
    InputIterator2 values_first,
    OutputIterator1 keys_output,
    OutputIterator2 values_output,
    BinaryFunction binary_op)
{
    typedef std::iterator_traits< InputIterator1 >::value_type kType;
    typedef std::iterator_traits< InputIterator2 >::value_type vType;
    typedef std::iterator_traits< OutputIterator1 >::value_type koType;
    typedef std::iterator_traits< OutputIterator2 >::value_type voType;
       static_assert( std::is_convertible< vType, voType >::value,
        "InputIterator2 and OutputIterator's value types are not convertible." );

   unsigned int numElements = static_cast< unsigned int >( std::distance( keys_first, keys_last ) );

    // do zeroeth element
    *values_output = *values_first;
    *keys_output = *keys_first;
    unsigned int count = 1;
    // rbk oneth element and beyond

    values_first++;
    for ( InputIterator1 key = (keys_first+1); key != keys_last; key++)
    {
        // load keys
        kType currentKey  = *(key);
        kType previousKey = *(key-1);

        // load value
        voType currentValue = *values_first;
        voType previousValue = *values_output; //Sure?: Damn sure

        previousValue = *values_output;
        // within segment
        if (currentKey == previousKey)
        {
            //std::cout << "continuing segment" << std::endl;
            voType r = binary_op( previousValue, currentValue);
            *values_output = r;
            *keys_output = currentKey;

        }
        else // new segment
        {
            values_output++;
            keys_output++;
            *values_output = currentValue;
            *keys_output = currentKey;
            count++; //To count the number of elements in the output array
        }
        values_first++;
    }

    //return result;
    //Print values here: Temporary, things gonna change after pair()

    //std::cout<<"Number of ele = "<<count<<std::endl;

    return std::make_pair(keys_output, values_output);


}

int _tmain(int argc, _TCHAR* argv[])
{

    int length = 10000;
    std::vector< int > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 1;
    std::vector< int > refInput( length );
    std::vector< int > input( length );
    for (int i = 0; i < length; i++)
    {
        if(std::rand()%3 == 1) key++;
        keys[i] = key;
        refInput[i] = std::rand()%4;
        input[i] = refInput[i];
    }

    // input and output vectors for device and reference

    std::vector< int > koutput( length );
    std::vector< int > voutput( length );
    //std::vector< int > refInput( length );
    std::vector< int > krefOutput( length );
    std::vector< int > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0);
    // call reduce_by_key

    bolt::cl::equal_to<int> binary_predictor;
    bolt::cl::plus<int> binary_operator;


    bolt::cl::reduce_by_key( keys.begin(), keys.end(), input.begin(), koutput.begin(), voutput.begin(),
                             binary_predictor, binary_operator);
    //for(unsigned int i = 0; i < 256 ; i++)
    //{
    //std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    //}
    gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(), vrefOutput.begin(),
                        std::plus<int>());

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);

    return 0;
}

#else

template<
    typename InputIterator1,
    typename InputIterator2,
    typename OutputIterator1,
    typename OutputIterator2,
    typename BinaryFunction>
std::pair<OutputIterator1, OutputIterator2>
gold_reduce_by_key( InputIterator1 keys_first,
                    InputIterator1 keys_last,
                    InputIterator2 values_first,
                    OutputIterator1 keys_output,
                    OutputIterator2 values_output,
                    BinaryFunction binary_op )
{
    typedef std::iterator_traits< InputIterator1 >::value_type kType;
    typedef std::iterator_traits< InputIterator2 >::value_type vType;
    typedef std::iterator_traits< OutputIterator1 >::value_type koType;
    typedef std::iterator_traits< OutputIterator2 >::value_type voType;
    static_assert( std::is_convertible< vType, voType >::value,
                   "InputIterator2 and OutputIterator's value types are not convertible." );

   unsigned int numElements = static_cast< unsigned int >( std::distance( keys_first, keys_last ) );

    // do zeroeth element
    *values_output = *values_first;
    *keys_output = *keys_first;
    unsigned int count = 1;
    // rbk oneth element and beyond

    values_first++;
    for ( InputIterator1 key = (keys_first+1); key != keys_last; key++)
    {
        // load keys
        kType currentKey  = *(key);
        kType previousKey = *(key-1);

        // load value
        voType currentValue = *values_first;
        voType previousValue = *values_output;

        previousValue = *values_output;
        // within segment
        if (currentKey == previousKey)
        {
            voType r = binary_op( previousValue, currentValue);
            *values_output = r;
            *keys_output = currentKey;

        }
        else // new segment
        {
            values_output++;
            keys_output++;
            *values_output = currentValue;
            *keys_output = currentKey;
            count++; //To count the number of elements in the output array
        }
        values_first++;
    }
    //std::cout<<count<<std::endl;
    return std::make_pair(keys_output+1, values_output+1);
}



class reduceStdVectorWithIters:public ::testing::TestWithParam<int>
{
protected:
    int myStdVectSize;
public:
    reduceStdVectorWithIters():myStdVectSize(GetParam()){
    }
};

typedef reduceStdVectorWithIters ReduceByKey;

typedef reduceStdVectorWithIters ReduceByKeyMultiCore;
typedef reduceStdVectorWithIters ReduceByKeyTest;

INSTANTIATE_TEST_CASE_P(ReduceByKeyIterFloatLimit, ReduceByKeyMultiCore, ::testing::Range(1025, 65535, 2111)); 
INSTANTIATE_TEST_CASE_P(ReduceByKeyIterFloatLimit, ReduceByKeyTest, ::testing::Range(1025, 65535, 2111)); 

TEST_P (ReduceByKeyTest, ReduceByKeyTestFloat)
{

    int length = myStdVectSize;
    std::vector< float > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    float key = 1.0f;
    std::vector< float > refInput( length );
    std::vector< float > input( length );
    for (int i = 0; i < length; i++)
    {
        if(std::rand()%4 == 1) key++;
        keys[i] = key;
        refInput[i] = (float)(std::rand()%4);
        input[i] = refInput[i];
    }

    // input and output vectors for device and reference

    std::vector< float > koutput( length );
    std::vector< float > voutput( length );
    std::vector< float > krefOutput( length );
    std::vector< float > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0.0f);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0.0f);
    
    bolt::cl::equal_to<float> binary_predictor;
    bolt::cl::plus<float> binary_operator;

    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( keys.begin(), keys.end(), input.begin(), koutput.begin(), voutput.begin(),
                                      binary_predictor, binary_operator);

#if 0

    for(unsigned int i = 0; i < 256 ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(),refInput.begin(),krefOutput.begin(),vrefOutput.begin(),
                                       std::plus<float>());
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);


}

TEST_P (ReduceByKeyTest, ReduceByKeyTestFloatIncreasingKeys)
{

    int length = myStdVectSize;
    std::vector< float > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    float key = 0.0f;
    std::vector<float>  refInput( length );
    std::vector<float>  input( length );
    //std::vector<int>  input( length );

    for (int i = 0; i < length; i++)
    { // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
            segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key;
        //device_keys[i] = key;
        segmentIndex++;

        refInput[i] = 3.0f;
        input[i] = refInput[i];
    }

    // input and output vectors for device and reference

    std::vector< float > koutput( length );
    std::vector< float > voutput( length );
    std::vector< float > krefOutput( length );
    std::vector< float > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0.0f);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0.0f);
    
    bolt::cl::equal_to<float> binary_predictor;
    bolt::cl::plus<float> binary_operator;

    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( keys.begin(), keys.end(), input.begin(), koutput.begin(), voutput.begin(),
                                      binary_predictor, binary_operator);

#if 0

    for(unsigned int i = 0; i < 256 ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(),refInput.begin(),krefOutput.begin(),vrefOutput.begin(),
                                       std::plus<float>());
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);


}


TEST_P (ReduceByKeyTest, ReduceByKeyTestFloat3)
{

    int length = myStdVectSize;
    std::vector< float > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    float key = 1.0f;
    std::vector< float > refInput( length );
    std::vector< float > input( length );
    for (int i = 0; i < length; i++)
    {
        if(std::rand()%3 == 1) key++;
        keys[i] = key;
        refInput[i] = (float)(std::rand()%4);
        input[i] = refInput[i];
    }

    // input and output vectors for device and reference

    std::vector< float > koutput( length );
    std::vector< float > voutput( length );
    std::vector< float > krefOutput( length );
    std::vector< float > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0.0f);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0.0f);
    
    bolt::cl::equal_to<float> binary_predictor;
    bolt::cl::plus<float> binary_operator;

    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( keys.begin(), keys.end(), input.begin(), koutput.begin(), voutput.begin(),
                                      binary_predictor, binary_operator);

#if 0

    for(unsigned int i = 0; i < 256 ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(),refInput.begin(),krefOutput.begin(),vrefOutput.begin(),
                                       std::plus<float>());
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);


}




TEST_P (ReduceByKeyTest, SameKeyReduceByKeyTestFloat)
{

    int length = myStdVectSize;
    std::vector< float > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    float key = 1.0f;
    std::vector< float > refInput( length );
    std::vector< float > input( length );
    for (int i = 0; i < length; i++)
    {
        keys[i] = 1;
        refInput[i] = (float)(std::rand()%4);
        input[i] = refInput[i];
    }

    // input and output vectors for device and reference

    std::vector< float > koutput( length );
    std::vector< float > voutput( length );
    std::vector< float > krefOutput( length );
    std::vector< float > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0.0f);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0.0f);
    
    bolt::cl::equal_to<float> binary_predictor;
    bolt::cl::plus<float> binary_operator;

    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( keys.begin(), keys.end(), input.begin(), koutput.begin(), voutput.begin(),
                                      binary_predictor, binary_operator);

#if 0

    for(unsigned int i = 0; i < 256 ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(),refInput.begin(),krefOutput.begin(),vrefOutput.begin(),
                                       std::plus<float>());
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);


}




TEST_P (ReduceByKeyTest, DifferentKeyReduceByKeyTestFloat)
{

    int length = myStdVectSize;
    std::vector< float > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    float key = 1.0f;
    std::vector< float > refInput( length );
    std::vector< float > input( length );
    for (int i = 0; i < length; i++)
    {
        keys[i] = i;
        refInput[i] = (float)(std::rand()%4);
        input[i] = refInput[i];
    }

    // input and output vectors for device and reference

    std::vector< float > koutput( length );
    std::vector< float > voutput( length );
    std::vector< float > krefOutput( length );
    std::vector< float > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0.0f);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0.0f);
    
    bolt::cl::equal_to<float> binary_predictor;
    bolt::cl::plus<float> binary_operator;

    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( keys.begin(), keys.end(), input.begin(), koutput.begin(), voutput.begin(),
                                      binary_predictor, binary_operator);

#if 0

    for(unsigned int i = 0; i < 256 ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(),refInput.begin(),krefOutput.begin(),vrefOutput.begin(),
                                       std::plus<float>());
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);


}

        










TEST(ReduceByKeyBasic, DeviceVectorIntTest)
{
    int length = 1<<10;
    std::vector< int > keys(length);
    bolt::cl::device_vector<int> device_keys(length);

    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 0;
    std::vector<int>  refInput( length );
    bolt::cl::device_vector<int>  input( length );

    for (int i = 0; i < length; i++)
    {   // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
            segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key;
        device_keys[i] = key;
        segmentIndex++;

        refInput[i] = i;
        input[i] = refInput[i];
    }
    // input and output vectors for device and reference
    bolt::cl::device_vector<int>  koutput( length );
    bolt::cl::device_vector<int>  voutput( length );
    std::vector<int>  krefOutput( length );
    std::vector<int>  vrefOutput( length );

    bolt::cl::equal_to<int> binary_predictor;
    bolt::cl::plus<int> binary_operator;
    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( device_keys.begin(), device_keys.end(), input.begin(), koutput.begin(), 
                                       voutput.begin(),binary_predictor, binary_operator);

    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(),refInput.begin(),krefOutput.begin(),
                                         vrefOutput.begin(), std::plus<int>());

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
TEST(ReduceByKeyBasic, OffSetDeviceVectorIntTest)
{
    int length = 1<<10;
    std::vector< int > keys(length);
    bolt::cl::device_vector<int> device_keys(length);

    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 0;
    std::vector<int>  refInput( length );
    bolt::cl::device_vector<int>  input( length );

    for (int i = 0; i < length; i++)
    {   // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
            segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key;
        device_keys[i] = key;
        segmentIndex++;

        refInput[i] = i;
        input[i] = refInput[i];
    }
    // input and output vectors for device and reference
    bolt::cl::device_vector<int>  koutput( length );
    bolt::cl::device_vector<int>  voutput( length );
    std::vector<int>  krefOutput( length );
    std::vector<int>  vrefOutput( length );

    bolt::cl::equal_to<int> binary_predictor;
    bolt::cl::plus<int> binary_operator;
    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( device_keys.begin()+10, device_keys.begin()+400, input.begin()+10, koutput.begin()+10, 
                                       voutput.begin()+10,binary_predictor, binary_operator);

    auto refPair = gold_reduce_by_key( keys.begin()+10, keys.begin()+400,refInput.begin()+10,krefOutput.begin()+10,
                                         vrefOutput.begin()+10, std::plus<int>());

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
TEST(ReduceByKeyBasic, SerialDeviceVectorIntTest)
{
    int length = 1<<10;
    std::vector< int > keys(length);
    bolt::cl::device_vector<int> device_keys(length);

    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 0;
    std::vector<int>  refInput( length );
    bolt::cl::device_vector<int>  input( length );

    for (int i = 0; i < length; i++)
    {   // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
            segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key;
        device_keys[i] = key;
        segmentIndex++;

        refInput[i] = i;
        input[i] = refInput[i];
    }
    // input and output vectors for device and reference
    bolt::cl::device_vector<int>  koutput( length );
    bolt::cl::device_vector<int>  voutput( length );
    std::vector<int>  krefOutput( length );
    std::vector<int>  vrefOutput( length );

    bolt::cl::equal_to<int> binary_predictor;
    bolt::cl::plus<int> binary_operator;
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::SerialCpu);
    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( ctl, device_keys.begin(), device_keys.end(), input.begin(),
                             koutput.begin(), voutput.begin(),binary_predictor, binary_operator);

    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(),refInput.begin(),krefOutput.begin(),
                                         vrefOutput.begin(),std::plus<int>());
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
TEST(ReduceByKeyBasic, MultiCoreDeviceVectorIntTest)
{
    int length = 1<<10;
    std::vector< int > keys(length);
    bolt::cl::device_vector<int> device_keys(length);
    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 0;
    std::vector<int>  refInput( length );
    bolt::cl::device_vector<int>  input( length );
    //std::vector<int>  input( length );

    for (int i = 0; i < length; i++)
    { // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
            segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key;
        device_keys[i] = key;
        segmentIndex++;

        refInput[i] = i;
        input[i] = refInput[i];
    }
    // input and output vectors for device and reference
    bolt::cl::device_vector<int>  koutput( length );
    bolt::cl::device_vector<int>  voutput( length );
    std::vector<int>  krefOutput( length );
    std::vector<int>  vrefOutput( length );
    bolt::cl::equal_to<int> binary_predictor;
    bolt::cl::plus<int> binary_operator;

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

for(int a=0; a<100; a++)
{
    auto p = bolt::cl::reduce_by_key( ctl, device_keys.begin(), device_keys.end(), input.begin(),
                               koutput.begin(), voutput.begin(), binary_predictor, binary_operator);

    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(),refInput.begin(),krefOutput.begin(),
                                           vrefOutput.begin(), binary_operator);
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
}

TEST(ReduceByKeyBasic, DeviceVectorFloatTest)
{
    int length = 1<<10;
    std::vector< float > keys(length);
    bolt::cl::device_vector<float> device_keys(length);

    int segmentLength = 0;
    int segmentIndex = 0;
    float key = 0.0f;
    std::vector<float>  refInput( length );
    bolt::cl::device_vector<float>  input( length );

    for (int i = 0; i < length; i++)
    {   // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
            segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key;
        device_keys[i] = key;
        segmentIndex++;

        refInput[i] = (float)i;
        input[i] = refInput[i];
    }
    // input and output vectors for device and reference
    bolt::cl::device_vector<float>  koutput( length );
    bolt::cl::device_vector<float>  voutput( length );
    std::vector<float>  krefOutput( length );
    std::vector<float>  vrefOutput( length );

    bolt::cl::equal_to<float> binary_predictor;
    bolt::cl::plus<float> binary_operator;
    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( device_keys.begin(), device_keys.end(), input.begin(), koutput.begin(), 
                                       voutput.begin(),binary_predictor, binary_operator);

    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(),refInput.begin(),krefOutput.begin(),
                                         vrefOutput.begin(), std::plus<float>());

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
TEST(ReduceByKeyBasic, OffSetDeviceVectorFloatTest)
{
    int length = 1<<10;
    std::vector< float > keys(length);
    bolt::cl::device_vector<float> device_keys(length);

    int segmentLength = 0;
    int segmentIndex = 0;
    float key = 0.0f;
    std::vector<float>  refInput( length );
    bolt::cl::device_vector<float>  input( length );

    for (int i = 0; i < length; i++)
    {   // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
            segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key;
        device_keys[i] = key;
        segmentIndex++;

        refInput[i] = (float)i;
        input[i] = refInput[i];
    }
    // input and output vectors for device and reference
    bolt::cl::device_vector<float>  koutput( length );
    bolt::cl::device_vector<float>  voutput( length );
    std::vector<float>  krefOutput( length );
    std::vector<float>  vrefOutput( length );

    bolt::cl::equal_to<float> binary_predictor;
    bolt::cl::plus<float> binary_operator;
    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( device_keys.begin()+10, device_keys.begin()+100, input.begin()+10, koutput.begin()+10, 
                                       voutput.begin()+10,binary_predictor, binary_operator);

    auto refPair = gold_reduce_by_key( keys.begin()+10, keys.begin()+100,refInput.begin()+10,krefOutput.begin()+10,
                                         vrefOutput.begin()+10, std::plus<float>());

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
TEST(ReduceByKeyBasic, SerialDeviceVectorFloatTest)
{
    int length = 1<<10;
    std::vector< float > keys(length);
    bolt::cl::device_vector<float> device_keys(length);

    int segmentLength = 0;
    int segmentIndex = 0;
    float key = 0.0f;
    std::vector<float>  refInput( length );
    bolt::cl::device_vector<float>  input( length );

    for (int i = 0; i < length; i++)
    {   // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
            segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key;
        device_keys[i] = key;
        segmentIndex++;

        refInput[i] = (float)i;
        input[i] = refInput[i];
    }
    // input and output vectors for device and reference
    bolt::cl::device_vector<float>  koutput( length );
    bolt::cl::device_vector<float>  voutput( length );
    std::vector<float>  krefOutput( length );
    std::vector<float>  vrefOutput( length );

    bolt::cl::equal_to<float> binary_predictor;
    bolt::cl::plus<float> binary_operator;
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::SerialCpu);
    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( ctl, device_keys.begin(), device_keys.end(), input.begin(),
                             koutput.begin(), voutput.begin(),binary_predictor, binary_operator);

    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(),refInput.begin(),krefOutput.begin(),
                                         vrefOutput.begin(),std::plus<float>());
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
TEST(ReduceByKeyBasic, MultiCoreDeviceVectorFloatTest)
{
    int length = 1<<12;
    std::vector< float > keys(length);
    bolt::cl::device_vector<float> device_keys(length);
    int segmentLength = 0;
    int segmentIndex = 0;
    float key = 0.0f;
    std::vector<float>  refInput( length );
    bolt::cl::device_vector<float>  input( length );
    //std::vector<int>  input( length );

    for (int i = 0; i < length; i++)
    { // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
            segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key;
        device_keys[i] = key;
        segmentIndex++;

        refInput[i] = (float)i;
        input[i] = refInput[i];
    }
    // input and output vectors for device and reference
    bolt::cl::device_vector<float>  koutput( length );
    bolt::cl::device_vector<float>  voutput( length );
    std::vector<float>  krefOutput( length );
    std::vector<float>  vrefOutput( length );
    bolt::cl::equal_to<float> binary_predictor;
    bolt::cl::plus<float> binary_operator;

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

for(int a=0; a<50; a++)
{
    auto p = bolt::cl::reduce_by_key( ctl, device_keys.begin(), device_keys.end(), input.begin(),
                               koutput.begin(), voutput.begin(), binary_predictor, binary_operator);

    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(),refInput.begin(),krefOutput.begin(),
                                           vrefOutput.begin(), std::plus<float>());
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
}

#if(TEST_DOUBLE == 1)
TEST(ReduceByKeyBasic, DeviceVectorDoubleTest)
{
    int length = 1<<10;
    std::vector< double > keys(length);
    bolt::cl::device_vector<double> device_keys(length);

    int segmentLength = 0;
    int segmentIndex = 0;
    double key = 0.0;
    std::vector<double>  refInput( length );
    bolt::cl::device_vector<double>  input( length );

    for (int i = 0; i < length; i++)
    {   // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
            segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key;
        device_keys[i] = key;
        segmentIndex++;

        refInput[i] = (double)i;
        input[i] = refInput[i];
    }
    // input and output vectors for device and reference
    bolt::cl::device_vector<double>  koutput( length );
    bolt::cl::device_vector<double>  voutput( length );
    std::vector<double>  krefOutput( length );
    std::vector<double>  vrefOutput( length );

    bolt::cl::equal_to<double> binary_predictor;
    bolt::cl::plus<double> binary_operator;
    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( device_keys.begin(), device_keys.end(), input.begin(), koutput.begin(), 
                                       voutput.begin(),binary_predictor, binary_operator);

    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(),refInput.begin(),krefOutput.begin(),
                                         vrefOutput.begin(), std::plus<double>());

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
TEST(ReduceByKeyBasic, OffSetDeviceVectorDoubleTest)
{
    int length = 1<<10;
    std::vector< double > keys(length);
    bolt::cl::device_vector<double> device_keys(length);

    int segmentLength = 0;
    int segmentIndex = 0;
    double key = 0.0;
    std::vector<double>  refInput( length );
    bolt::cl::device_vector<double>  input( length );

    for (int i = 0; i < length; i++)
    {   // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
            segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key;
        device_keys[i] = key;
        segmentIndex++;

        refInput[i] = (double)i;
        input[i] = refInput[i];
    }
    // input and output vectors for device and reference
    bolt::cl::device_vector<double>  koutput( length );
    bolt::cl::device_vector<double>  voutput( length );
    std::vector<double>  krefOutput( length );
    std::vector<double>  vrefOutput( length );

    bolt::cl::equal_to<double> binary_predictor;
    bolt::cl::plus<double> binary_operator;
    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( device_keys.begin()+10, device_keys.begin()+200, input.begin()+10, koutput.begin()+10, 
                                       voutput.begin()+10,binary_predictor, binary_operator);

    auto refPair = gold_reduce_by_key( keys.begin()+10, keys.begin()+200,refInput.begin()+10,krefOutput.begin()+10,
                                         vrefOutput.begin()+10, std::plus<double>());

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}

TEST(ReduceByKeyBasic, CPUDeviceVectorDoubleTest)
{
    int length = 1<<10;
    std::vector< double > keys(length);
    bolt::cl::device_vector<double> device_keys(length);

    int segmentLength = 0;
    int segmentIndex = 0;
    double key = 0.0;
    std::vector<double>  refInput( length );
    bolt::cl::device_vector<double>  input( length );

    for (int i = 0; i < length; i++)
    {   // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
            segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key;
        device_keys[i] = key;
        segmentIndex++;

        refInput[i] = (double)i;
        input[i] = refInput[i];
    }
    // input and output vectors for device and reference
    bolt::cl::device_vector<double>  koutput( length );
    bolt::cl::device_vector<double>  voutput( length );
    std::vector<double>  krefOutput( length );
    std::vector<double>  vrefOutput( length );

    bolt::cl::equal_to<double> binary_predictor;
    bolt::cl::plus<double> binary_operator;
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::SerialCpu);
    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( ctl, device_keys.begin(), device_keys.end(), input.begin(),
                             koutput.begin(), voutput.begin(),binary_predictor, binary_operator);

    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(),refInput.begin(),krefOutput.begin(),
                                         vrefOutput.begin(),std::plus<double>());
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
TEST(ReduceByKeyBasic, MultiCoreCPUDeviceVectorDoubleTest)
{
    int length = 1<<12;
    std::vector< double > keys(length);
    bolt::cl::device_vector<double> device_keys(length);
    int segmentLength = 0;
    int segmentIndex = 0;
    double key = 0.0;
    std::vector<double>  refInput( length );
    bolt::cl::device_vector<double>  input( length );
    //std::vector<int>  input( length );

    for (int i = 0; i < length; i++)
    { // start over, i.e., begin assigning new key
        if (segmentIndex == segmentLength)
        {
            segmentLength++;
            segmentIndex = 0;
            ++key;
        }
        keys[i] = key;
        device_keys[i] = key;
        segmentIndex++;

        refInput[i] = (double)i;
        input[i] = refInput[i];
    }
    // input and output vectors for device and reference
    bolt::cl::device_vector<double>  koutput( length );
    bolt::cl::device_vector<double>  voutput( length );
    std::vector<double>  krefOutput( length );
    std::vector<double>  vrefOutput( length );
    bolt::cl::equal_to<double> binary_predictor;
    bolt::cl::plus<double> binary_operator;

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

for(int a=0; a<50; a++)
{
    auto p = bolt::cl::reduce_by_key( ctl, device_keys.begin(), device_keys.end(), input.begin(),
                               koutput.begin(), voutput.begin(), binary_predictor, binary_operator);

    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(),refInput.begin(),krefOutput.begin(),
                                           vrefOutput.begin(), std::plus<double>());
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
}
#endif
 
TEST(reduce_by_key__bolt_Std_vect, Basic_EPR377067)
{
    int size = 10;
    std::vector<int> vectKeyIn(size);
    std::vector<int> vectValueIn(size);
    std::vector<int> keyBoltClDevVectOp(size);
    std::vector<int> valueBoltClDevVectOp(size);

    for (int i = 0; i < std::ceil(size/3.0); i++)
        vectKeyIn[i] = (int)2;
    for (int i = (int)(std::ceil(size/3.0) + 1); i < std::ceil((2* size)/3.0); i++)
        vectKeyIn[i] = (int)3;
    for (int i =  (int)(std::ceil((2* size)/3.0) + 1); i < size; i++)
        vectKeyIn[i] = (int)5;
    //now elemetns in vectKeyIn are as: {2 2 2 2 0 3 3 0 5 5 }

    for (int i = 0; i < size; i++)
        vectValueIn[i] = (int) i; //elements in vectValueIn are as: {0 1 2 3 4 5 6 7 8 9}

    bolt::cl::equal_to<int> eq;
    bolt::cl::reduce_by_key(vectKeyIn.begin(), vectKeyIn.end(), vectValueIn.begin(), keyBoltClDevVectOp.begin(),
                                                                            valueBoltClDevVectOp.begin(), eq);
    int eleKeyOp_Expexted[5] = {2, 0, 3, 0, 5};
    int eleValueOp_Expexted[5] = {6, 4, 11, 7, 17};
    for (int i = 0; i < 5; i++)
    {
        EXPECT_EQ ( eleKeyOp_Expexted[i], keyBoltClDevVectOp[i]);
        EXPECT_EQ ( eleValueOp_Expexted[i], valueBoltClDevVectOp[i]);
    }
}
TEST(reduce_by_key__bolt_Std_vect, SerialBasic_EPR377067)
    {
    int size = 10;
    std::vector<int> vectKeyIn(size);
    std::vector<int> vectValueIn(size);
    std::vector<int> keyBoltClDevVectOp(size);
    std::vector<int> valueBoltClDevVectOp(size);

    for (int i = 0; i < std::ceil(size/3.0); i++)
        vectKeyIn[i] = (int)2;
    for (int i = (int)(std::ceil(size/3.0) + 1); i < std::ceil((2* size)/3.0); i++)
        vectKeyIn[i] = (int)3;
    for (int i =  (int)(std::ceil((2* size)/3.0) + 1); i < size; i++)
        vectKeyIn[i] = (int)5;
    //now elemetns in vectKeyIn are as: {2 2 2 2 0 3 3 0 5 5 }

    for (int i = 0; i < size; i++)
        vectValueIn[i] = (int) i; //elements in vectValueIn are as: {0 1 2 3 4 5 6 7 8 9}

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::SerialCpu);

    bolt::cl::equal_to<int> eq;
    bolt::cl::reduce_by_key(ctl, vectKeyIn.begin(), vectKeyIn.end(), vectValueIn.begin(),
                           keyBoltClDevVectOp.begin(),valueBoltClDevVectOp.begin(), eq);

    int eleKeyOp_Expexted[5] = {2, 0, 3, 0, 5};
    int eleValueOp_Expexted[5] = {6, 4, 11, 7, 17};

    for (int i = 0; i < 5; i++)
    {
        EXPECT_EQ ( eleKeyOp_Expexted[i], keyBoltClDevVectOp[i]);
        EXPECT_EQ ( eleValueOp_Expexted[i], valueBoltClDevVectOp[i]);
    }
}
TEST(reduce_by_key__bolt_Std_vect, MulticoreBasic_EPR377067)
{
    int size = 10;
    std::vector<int> vectKeyIn(size);
    std::vector<int> vectValueIn(size);
    std::vector<int> keyBoltClDevVectOp(size);
    std::vector<int> valueBoltClDevVectOp(size);

    for (int i = 0; i < std::ceil(size/3.0); i++)
        vectKeyIn[i] = (int)2;
    for (int i = (int)(std::ceil(size/3.0) + 1); i < std::ceil((2* size)/3.0); i++)
        vectKeyIn[i] = (int)3;
    for (int i =  (int)(std::ceil((2* size)/3.0) + 1); i < size; i++)
        vectKeyIn[i] = (int)5;
    //now elemetns in vectKeyIn are as: {2 2 2 2 0 3 3 0 5 5 }

    for (int i = 0; i < size; i++)
        vectValueIn[i] = (int) i; //elements in vectValueIn are as: {0 1 2 3 4 5 6 7 8 9}

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);
    bolt::cl::equal_to<int> eq;
    bolt::cl::reduce_by_key(ctl, vectKeyIn.begin(), vectKeyIn.end(), vectValueIn.begin(), keyBoltClDevVectOp.begin(),
                                                                            valueBoltClDevVectOp.begin(), eq);
    int eleKeyOp_Expexted[5] = {2, 0, 3, 0, 5};
    int eleValueOp_Expexted[5] = {6, 4, 11, 7, 17};

    for (int i = 0; i < 5; i++)
    {
        EXPECT_EQ ( eleKeyOp_Expexted[i], keyBoltClDevVectOp[i]);
        EXPECT_EQ ( eleValueOp_Expexted[i], valueBoltClDevVectOp[i]);
    }
}

TEST(reduce_by_key__bolt_Dev_vect, Basic_EPR377067)
{
    int size = 10;
    bolt::cl::device_vector<int> vectKeyIn(size);
    bolt::cl::device_vector<int> vectValueIn(size);
    bolt::cl::device_vector<int> keyBoltClDevVectOp(size);
    bolt::cl::device_vector<int> valueBoltClDevVectOp(size);

    for (int i = 0; i < std::ceil(size/3.0); i++)
        vectKeyIn[i] = (int)2;
    for (int i = (int)(std::ceil(size/3.0) + 1); i < std::ceil((2* size)/3.0); i++)
        vectKeyIn[i] = (int)3;
    for (int i = (int)(std::ceil((2* size)/3.0) + 1); i < size; i++)
        vectKeyIn[i] = (int)5;
    
    //now elemetns in vectKeyIn are as: {2 2 2 2 0 3 3 0 5 5 }
    for (int i = 0; i < size; i++)
        vectValueIn[i] = (int) i; //elements in vectValueIn are as: {0 1 2 3 4 5 6 7 8 9}

    bolt::cl::equal_to<int> eq;
    bolt::cl::reduce_by_key(vectKeyIn.begin(), vectKeyIn.end(), vectValueIn.begin(), keyBoltClDevVectOp.begin(),
                                                                                valueBoltClDevVectOp.begin(), eq);
    int eleKeyOp_Expexted[5] = {2, 0, 3, 0, 5};
    int eleValueOp_Expexted[5] = {6, 4, 11, 7, 17};
    for (int i = 0; i < 5; i++)
    {
        EXPECT_EQ ( eleKeyOp_Expexted[i], keyBoltClDevVectOp[i]);
        EXPECT_EQ ( eleValueOp_Expexted[i], valueBoltClDevVectOp[i]);
    }
}
TEST(reduce_by_key__bolt_Dev_vect, SerialBasic_EPR377067)
{
    int size = 10;
    bolt::cl::device_vector<int> vectKeyIn(size);
    bolt::cl::device_vector<int> vectValueIn(size);
    bolt::cl::device_vector<int> keyBoltClDevVectOp(size);
    bolt::cl::device_vector<int> valueBoltClDevVectOp(size);

    for(int i = 0; i < std::ceil(size/3.0); i++)
        vectKeyIn[i] = (int)2;
    for(int i = (int)(std::ceil(size/3.0) + 1); i < std::ceil((2* size)/3.0); i++)
        vectKeyIn[i] = (int)3;
    for(int i = (int)(std::ceil((2* size)/3.0) + 1); i < size; i++)
        vectKeyIn[i] = (int)5;
    //now elemetns in vectKeyIn are as: {2 2 2 2 0 3 3 0 5 5 }

    for (int i = 0; i < size; i++)
    vectValueIn[i] = (int) i; //elements in vectValueIn are as: {0 1 2 3 4 5 6 7 8 9}

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::SerialCpu);
    bolt::cl::equal_to<int> eq;
    bolt::cl::reduce_by_key(ctl, vectKeyIn.begin(), vectKeyIn.end(), vectValueIn.begin(), keyBoltClDevVectOp.begin(),
                                                                                valueBoltClDevVectOp.begin(), eq);
    int eleKeyOp_Expexted[5] = {2, 0, 3, 0, 5};
    int eleValueOp_Expexted[5] = {6, 4, 11, 7, 17};

    for (int i = 0; i < 5; i++)
    {
        EXPECT_EQ ( eleKeyOp_Expexted[i], keyBoltClDevVectOp[i]);
        EXPECT_EQ ( eleValueOp_Expexted[i], valueBoltClDevVectOp[i]);
    }
}
TEST(reduce_by_key__bolt_Dev_vect, MulticoreBasic_EPR377067)
{
    int size = 10;
    bolt::cl::device_vector<int> vectKeyIn(size);
    bolt::cl::device_vector<int> vectValueIn(size);
    bolt::cl::device_vector<int> keyBoltClDevVectOp(size);
    bolt::cl::device_vector<int> valueBoltClDevVectOp(size);

    for (int i = 0; i < std::ceil(size/3.0); i++)
        vectKeyIn[i] = (int)2;
    for (int i = (int)(std::ceil(size/3.0) + 1); i < std::ceil((2* size)/3.0); i++)
        vectKeyIn[i] = (int)3;
    for (int i = (int)(std::ceil((2* size)/3.0) + 1); i < size; i++)
        vectKeyIn[i] = (int)5;
    //now elemetns in vectKeyIn are as: {2 2 2 2 0 3 3 0 5 5 }

    for (int i = 0; i < size; i++)
    vectValueIn[i] = (int) i; //elements in vectValueIn are as: {0 1 2 3 4 5 6 7 8 9}

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);
    bolt::cl::equal_to<int> eq;
    bolt::cl::reduce_by_key(ctl, vectKeyIn.begin(), vectKeyIn.end(), vectValueIn.begin(), keyBoltClDevVectOp.begin(),
                                                                                valueBoltClDevVectOp.begin(), eq);
    int eleKeyOp_Expexted[5] = {2, 0, 3, 0, 5};
    int eleValueOp_Expexted[5] = {6, 4, 11, 7, 17};

    for (int i = 0; i < 5; i++)
    {
        EXPECT_EQ ( eleKeyOp_Expexted[i], keyBoltClDevVectOp[i]);
        EXPECT_EQ ( eleValueOp_Expexted[i], valueBoltClDevVectOp[i]);
    }
}

TEST(ReduceByKeyBasic, IntegerTest)
{
    int length = 1<<15;
    std::vector< int > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 1;
    std::vector< int > refInput( length );
    std::vector< int > input( length );
    for (int i = 0; i < length; i++)
    {
        if(std::rand()%4 == 1) key++;
        keys[i] = key;
        refInput[i] = std::rand()%4;
        input[i] = refInput[i];
    }

    // input and output vectors for device and reference

    std::vector< int > koutput( length );
    std::vector< int > voutput( length );
    std::vector< int > krefOutput( length );
    std::vector< int > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0);


    bolt::cl::equal_to<int> binary_predictor;
    bolt::cl::plus<int> binary_operator;


    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( keys.begin(), keys.end(), input.begin(), koutput.begin(), voutput.begin(),
                                      binary_predictor, binary_operator);

#if 0

    for(unsigned int i = 0; i < 256 ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(),refInput.begin(),krefOutput.begin(),vrefOutput.begin(),
                                       std::plus<int>());

    //cmpArrays2(krefOutput, koutput, refPair.first, p.first);
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
   // cmpArrays2(vrefOutput, voutput, refPair.second, p.second);
}
TEST(ReduceByKeyBasic, IntegerTestOffsetTest)
{
    int length = 1<<15;
    std::vector< int > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 1;
    std::vector< int > refInput( length );
    std::vector< int > input( length );
    for (int i = 0; i < length; i++)
    {
        if(std::rand()%4 == 1) key++;
        keys[i] = key;
        refInput[i] = std::rand()%4;
        input[i] = refInput[i];
    }

    // input and output vectors for device and reference

    std::vector< int > koutput( length );
    std::vector< int > voutput( length );
    std::vector< int > krefOutput( length );
    std::vector< int > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0);


    bolt::cl::equal_to<int> binary_predictor;
    bolt::cl::plus<int> binary_operator;


    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( keys.begin() +10, keys.begin()+400, input.begin()+10, koutput.begin() +10, voutput.begin() +10,
                                      binary_predictor, binary_operator);

#if 0

    for(unsigned int i = 0; i < 256 ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    auto refPair = gold_reduce_by_key( keys.begin() +10, keys.begin() +400, refInput.begin()+10,krefOutput.begin()+10,vrefOutput.begin()+10,
                                       std::plus<int>());

    //cmpArrays2(krefOutput, koutput, refPair.first, p.first);
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
   // cmpArrays2(vrefOutput, voutput, refPair.second, p.second);
}
TEST(ReduceByKeyBasic, SerialIntegerTest)
{
    int length = 1<<24;
    std::vector< int > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 1;
    std::vector< int > refInput( length );
    std::vector< int > input( length );
    for (int i = 0; i < length; i++)
    {
        if(std::rand()%3 == 1) key++;
        keys[i] = key;
        refInput[i] = std::rand()%4;
        input[i] = refInput[i];
    }

    // input and output vectors for device and reference

    std::vector< int > koutput( length );
    std::vector< int > voutput( length );
    std::vector< int > krefOutput( length );
    std::vector< int > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0);


    bolt::cl::equal_to<int> binary_predictor;
    bolt::cl::plus<int> binary_operator;

    ::cl::Context myContext = bolt::cl::control::getDefault( ).getContext( );
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::SerialCpu);

    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( ctl, keys.begin(), keys.end(), input.begin(), koutput.begin(), voutput.begin(),
                                      binary_predictor, binary_operator);

#if 0

    for(unsigned int i = 0; i < 256 ; i++)
    {
     std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(),
                                       vrefOutput.begin(),std::plus<int>());

    //cmpArrays2(krefOutput, koutput, refPair.first, p.first);
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
   // cmpArrays2(vrefOutput, voutput, refPair.second, p.second);
}
TEST(ReduceByKeyBasic, MultiCoreIntegerTest)
{
    int length = 1<<10;
    std::vector< int > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 1;
    std::vector< int > refInput( length );
    std::vector< int > input( length );
    for (int i = 0; i < length; i++)
    {
        if(std::rand()%4 == 1) key++;
        keys[i] = key;
        refInput[i] = 3;
        input[i] = refInput[i];
    }
    // input and output vectors for device and reference

    std::vector< int > koutput( length );
    std::vector< int > voutput( length );
    std::vector< int > krefOutput( length );
    std::vector< int > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0);

    bolt::cl::equal_to<int> binary_predictor;
    bolt::cl::plus<int> binary_operator;
       
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

for(int i=0;i<100;i++)
{   // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( ctl, keys.begin(), keys.end(), input.begin(), koutput.begin(), voutput.begin(),
                                      binary_predictor, binary_operator);
#if 0
    for(unsigned int i = 0; i < 256 ; i++)
    {
     std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }
#endif
    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(),refInput.begin(),krefOutput.begin(),vrefOutput.begin(),
                                       std::plus<int>());
    
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
}

TEST(ReduceByKeyBasic, FloatTest)
{
    int length = 1<<15;
    std::vector< float > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    float key = 1.0f;
    std::vector< float > refInput( length );
    std::vector< float > input( length );
    for (int i = 0; i < length; i++)
    {
        if(std::rand()%4 == 1) key++;
        keys[i] = key;
        refInput[i] = (float)(std::rand()%4);
        input[i] = refInput[i];
    }

    // input and output vectors for device and reference

    std::vector< float > koutput( length );
    std::vector< float > voutput( length );
    std::vector< float > krefOutput( length );
    std::vector< float > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0.0f);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0.0f);
    
    bolt::cl::equal_to<float> binary_predictor;
    bolt::cl::plus<float> binary_operator;

    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( keys.begin(), keys.end(), input.begin(), koutput.begin(), voutput.begin(),
                                      binary_predictor, binary_operator);

#if 0

    for(unsigned int i = 0; i < 256 ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(),refInput.begin(),krefOutput.begin(),vrefOutput.begin(),
                                       std::plus<float>());
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
TEST(ReduceByKeyBasic, FloatTestOffsetTest)
{
    int length = 1<<15;
    std::vector< float > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    float key = 1.0f;
    std::vector< float > refInput( length );
    std::vector< float > input( length );
    for (int i = 0; i < length; i++)
    {
        if(std::rand()%3 == 1) key++;
        keys[i] = key;
        refInput[i] = (float)(std::rand()%4);
        input[i] = refInput[i];
    }

    // input and output vectors for device and reference

    std::vector< float > koutput( length );
    std::vector< float > voutput( length );
    std::vector< float > krefOutput( length );
    std::vector< float > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0.0f);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0.0f);

    bolt::cl::equal_to<float> binary_predictor;
    bolt::cl::plus<float> binary_operator;
    
    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( keys.begin() +10, keys.begin()+400, input.begin()+10, koutput.begin() +10, voutput.begin() +10,
                                      binary_predictor, binary_operator);

#if 0

    for(unsigned int i = 0; i < 256 ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    auto refPair = gold_reduce_by_key( keys.begin() +10, keys.begin() +400, refInput.begin()+10,krefOutput.begin()+10,vrefOutput.begin()+10,
                                       std::plus<float>());
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
TEST(ReduceByKeyBasic, SerialFloatTest)
{
    int length = 1<<10;
    std::vector< float > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    float key = 1.0f;
    std::vector< float > refInput( length );
    std::vector< float > input( length );
    for (int i = 0; i < length; i++)
    {
        if(std::rand()%3 == 1) key++;
        keys[i] = key;
        refInput[i] = (float)(std::rand()%4);
        input[i] = refInput[i];
    }

    // input and output vectors for device and reference

    std::vector< float > koutput( length );
    std::vector< float > voutput( length );
    std::vector< float > krefOutput( length );
    std::vector< float > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0.0f);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0.0f);


    bolt::cl::equal_to<float> binary_predictor;
    bolt::cl::plus<float> binary_operator;

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::SerialCpu);

    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( ctl, keys.begin(), keys.end(), input.begin(), koutput.begin(), voutput.begin(),
                                      binary_predictor, binary_operator);

    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(),
                                       vrefOutput.begin(),std::plus<float>());
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
TEST(ReduceByKeyBasic, MultiCoreFloatTest)
{
    int length = 1<<10;
    std::vector< float > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    float key = 1.0f;
    std::vector< float > refInput( length );
    std::vector< float > input( length );
    for (int i = 0; i < length; i++)
    {
        if(std::rand()%3 == 1) key++;
        keys[i] = key;
        refInput[i] = 3;
        input[i] = refInput[i];
    }
    // input and output vectors for device and reference

    std::vector< float > koutput( length );
    std::vector< float > voutput( length );
    std::vector< float > krefOutput( length );
    std::vector< float > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0.0f);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0.0f);

    bolt::cl::equal_to<float> binary_predictor;
    bolt::cl::plus<float> binary_operator;
       
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

for(int i=0;i<100;i++)
{   // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( ctl, keys.begin(), keys.end(), input.begin(), koutput.begin(), voutput.begin(),
                                      binary_predictor, binary_operator);
#if 0
    for(unsigned int i = 0; i < 256 ; i++)
    {
     std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }
#endif
    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(),refInput.begin(),krefOutput.begin(),vrefOutput.begin(),
                                       std::plus<float>());    
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
}

#if(TEST_DOUBLE == 1)

TEST(ReduceByKeyBasic, DoubleTest)
{
    int length = 1<<24;
    std::vector< double > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    double key = 1.0;
    std::vector< double > refInput( length );
    std::vector< double > input( length );
    for (int i = 0; i < length; i++)
    {
        if(std::rand()%3 == 1) key++;
        keys[i] = key;
        refInput[i] = (double)(std::rand()%4);
        input[i] = refInput[i];
    }

    // input and output vectors for device and reference

    std::vector< double > koutput( length );
    std::vector< double > voutput( length );
    std::vector< double > krefOutput( length );
    std::vector< double > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0.0);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0.0);
    
    bolt::cl::equal_to<double> binary_predictor;
    bolt::cl::plus<double> binary_operator;

    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( keys.begin(), keys.end(), input.begin(), koutput.begin(), voutput.begin(),
                                      binary_predictor, binary_operator);

#if 0

    for(unsigned int i = 0; i < 256 ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(),refInput.begin(),krefOutput.begin(),vrefOutput.begin(),
                                       std::plus<double>());
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
TEST(ReduceByKeyBasic, OffSetDoubleTest)
{
    int length = 1<<24;
    std::vector< double > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    double key = 1.0;
    std::vector< double > refInput( length );
    std::vector< double > input( length );
    for (int i = 0; i < length; i++)
    {
        if(std::rand()%4 == 1) key++;
        keys[i] = key;
        refInput[i] = (double)(std::rand()%4);
        input[i] = refInput[i];
    }

    // input and output vectors for device and reference

    std::vector< double > koutput( length );
    std::vector< double > voutput( length );
    std::vector< double > krefOutput( length );
    std::vector< double > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0.0);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0.0);
    
    bolt::cl::equal_to<double> binary_predictor;
    bolt::cl::plus<double> binary_operator;

    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( keys.begin()+10, keys.begin()+200, input.begin()+10, koutput.begin()+10, 
                                      voutput.begin()+10, binary_predictor, binary_operator);
#if 0

    for(unsigned int i = 0; i < 256 ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif
    auto refPair = gold_reduce_by_key( keys.begin()+10, keys.begin()+200,refInput.begin()+10,krefOutput.begin()+10,
                                      vrefOutput.begin()+10,std::plus<double>());
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}

TEST(ReduceByKeyBasic, DoubleTestOffsetTest)
{
    int length = 1<<15;
    std::vector< double > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    double key = 1.0;
    std::vector< double > refInput( length );
    std::vector< double > input( length );
    for (int i = 0; i < length; i++)
    {
        if(std::rand()%4 == 1) key++;
        keys[i] = key;
        refInput[i] = (double)(std::rand()%4);
        input[i] = refInput[i];
    }

    // input and output vectors for device and reference

    std::vector< double > koutput( length );
    std::vector< double > voutput( length );
    std::vector< double > krefOutput( length );
    std::vector< double > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0.0);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0.0);

    bolt::cl::equal_to<double> binary_predictor;
    bolt::cl::plus<double> binary_operator;
    
    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( keys.begin() +10, keys.begin()+400, input.begin()+10, koutput.begin() +10, voutput.begin() +10,
                                      binary_predictor, binary_operator);

#if 0

    for(unsigned int i = 0; i < 256 ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    auto refPair = gold_reduce_by_key( keys.begin() +10, keys.begin() +400, refInput.begin()+10,krefOutput.begin()+10,vrefOutput.begin()+10,
                                       std::plus<double>());
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
TEST(ReduceByKeyBasic, SerialDoubleTest)
{
    int length = 1<<24;
    std::vector< double > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    double key = 1.0;
    std::vector< double > refInput( length );
    std::vector< double > input( length );
    for (int i = 0; i < length; i++)
    {
        if(std::rand()%3 == 1) key++;
        keys[i] = key;
        refInput[i] = (double)(std::rand()%4);
        input[i] = refInput[i];
    }

    // input and output vectors for device and reference

    std::vector< double > koutput( length );
    std::vector< double > voutput( length );
    std::vector< double > krefOutput( length );
    std::vector< double > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0.0);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0.0);


    bolt::cl::equal_to<double> binary_predictor;
    bolt::cl::plus<double> binary_operator;

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::SerialCpu);

    // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( ctl, keys.begin(), keys.end(), input.begin(), koutput.begin(), voutput.begin(),
                                      binary_predictor, binary_operator);

#if 0

    for(unsigned int i = 0; i < 256 ; i++)
    {
     std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(),
                                       vrefOutput.begin(),std::plus<double>());
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
TEST(ReduceByKeyBasic, MultiCoreDoubleTest)
{
    int length = 1<<10;
    std::vector< double > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    double key = 1.0;
    std::vector< double > refInput( length );
    std::vector< double > input( length );
    for (int i = 0; i < length; i++)
    {
        if(std::rand()%3 == 1) key++;
        keys[i] = key;
        refInput[i] = 3;
        input[i] = refInput[i];
    }
    // input and output vectors for device and reference

    std::vector< double > koutput( length );
    std::vector< double > voutput( length );
    std::vector< double > krefOutput( length );
    std::vector< double > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0.0);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0.0);

    bolt::cl::equal_to<double> binary_predictor;
    bolt::cl::plus<double> binary_operator;
       
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

for(int i=0;i<100;i++)
{   // call reduce_by_key
    auto p = bolt::cl::reduce_by_key( ctl, keys.begin(), keys.end(), input.begin(), koutput.begin(), voutput.begin(),
                                      binary_predictor, binary_operator);
#if 0
    for(unsigned int i = 0; i < 256 ; i++)
    {
     std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }
#endif
    auto refPair = gold_reduce_by_key( keys.begin(), keys.end(),refInput.begin(),krefOutput.begin(),vrefOutput.begin(),
                                       std::plus<double>());    
    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
}

#endif

TEST(ReduceByKeyPairCheck, IntegerTest2)
{
    int length = 1<<15;
    std::vector< int > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 1;
    std::vector< int > refInput( length );
    std::vector< int > input( length );
	
for(int a=0; a<100; a++)
{
//	std::srand(a+12); 
    for (int i = 0; i < length; i++)
    {
        if(std::rand()%3 == 1) key++;
        keys[i] = key;
        refInput[i] = std::rand()%4;
        input[i] = refInput[i];
    }keys[1] = keys[2] = keys[3] = keys[4] =1;
    keys[5] = keys[6] = keys[7] = keys[8] =2; keys[9] = 3;

    // input and output vectors for device and reference

    std::vector< int > koutput( length );
    std::vector< int > voutput( length );
    std::vector< int > krefOutput( length );
    std::vector< int > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0);
    std::fill(koutput.begin(),koutput.end(),0);
    std::fill(voutput.begin(),voutput.end(),0);

    bolt::cl::equal_to<int> binary_predictor;
    bolt::cl::plus<int> binary_operator;

    typedef std::pair<std::vector<int>::iterator,std::vector<int>::iterator> StdPairIterator;
    typedef bolt::cl::pair<std::vector<int>::iterator,std::vector<int>::iterator> DevicePairIterator;

    // call reduce_by_key
    DevicePairIterator dv_pair =  bolt::cl::reduce_by_key(keys.begin(), keys.end(),input.begin(),
                                   koutput.begin(), voutput.begin(), binary_predictor,binary_operator);
#if 1
    for(unsigned int i = 0; i < 30 ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }
#endif

    DevicePairIterator gold_pair =
    gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(),
                        vrefOutput.begin(),std::plus<int>());

    size_t sizeAfterCall = gold_pair.first - krefOutput.begin();
    size_t sizeAfterDeviceCall = dv_pair.first - koutput.begin();

    std::cout<<sizeAfterCall<<" Is the gold key size after call! " <<length<<std::endl;
    std::cout<<sizeAfterDeviceCall<<" Is the dv key size after call!"<<std::endl;

    krefOutput.resize(sizeAfterCall);
    vrefOutput.resize(sizeAfterCall);
    koutput.resize(sizeAfterDeviceCall);
    voutput.resize(sizeAfterDeviceCall);

#if 0

    for(unsigned int i = 0; i < sizeAfterDeviceCall ; i++)
    {
        std::cout<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}

}
TEST(ReduceByKeyPairCheck, OffSetIntegerTest2)
{
    int length = 1<<10;
    std::vector< int > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 1;
    std::vector< int > refInput( length );
    std::vector< int > input( length );

    for (int i = 0; i < length; i++)
    {
        if(std::rand()%3 == 1) key++;
        keys[i] = key;
        refInput[i] = std::rand()%4;
        input[i] = refInput[i];
    }keys[1] = keys[2] = keys[3] = keys[4] =1;
    keys[5] = keys[6] = keys[7] = keys[8] =2; keys[9] = 3;

    // input and output vectors for device and reference

    std::vector< int > koutput( length );
    std::vector< int > voutput( length );
    std::vector< int > krefOutput( length );
    std::vector< int > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0);
    std::fill(koutput.begin(),koutput.end(),0);
    std::fill(voutput.begin(),voutput.end(),0);

    bolt::cl::equal_to<int> binary_predictor;
    bolt::cl::plus<int> binary_operator;

    typedef std::pair<std::vector<int>::iterator,std::vector<int>::iterator> StdPairIterator;
    typedef bolt::cl::pair<std::vector<int>::iterator,std::vector<int>::iterator> DevicePairIterator;

    // call reduce_by_key
    DevicePairIterator dv_pair =
    bolt::cl::reduce_by_key(keys.begin()+10, keys.begin()+400,input.begin()+10,koutput.begin()+10,voutput.begin()+10,
                              binary_predictor, binary_operator);

#if 0

    for(unsigned int i = 0; i < length ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    DevicePairIterator gold_pair =
    gold_reduce_by_key( keys.begin()+10, keys.begin()+400, refInput.begin()+10, krefOutput.begin()+10,
                        vrefOutput.begin()+10,std::plus<int>());

    size_t sizeAfterCall = gold_pair.first - krefOutput.begin();
    size_t sizeAfterDeviceCall = dv_pair.first - koutput.begin();

    std::cout<<sizeAfterCall<<" Is the gold key size after call!"<<std::endl;
    std::cout<<sizeAfterDeviceCall<<" Is the dv key size after call!"<<std::endl;

    krefOutput.resize(sizeAfterCall);
    vrefOutput.resize(sizeAfterCall);
    koutput.resize(sizeAfterDeviceCall);
    voutput.resize(sizeAfterDeviceCall);

#if 0

    for(unsigned int i = 0; i < sizeAfterDeviceCall ; i++)
    {
        std::cout<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
TEST(ReduceByKeyPairCheck, SerialIntegerTest2)
{
    int length = 1<<10;
    std::vector< int > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 1;
    std::vector< int > refInput( length );
    std::vector< int > input( length );

    for (int i = 0; i < length; i++)
    {
        if(std::rand()%3 == 1) key++;
        keys[i] = key;
        refInput[i] = std::rand()%4;
        input[i] = refInput[i];
    }keys[1] = keys[2] = keys[3] = keys[4] =1;
    keys[5] = keys[6] = keys[7] = keys[8] =2; keys[9] = 3;

    // input and output vectors for device and reference

    std::vector< int > koutput( length );
    std::vector< int > voutput( length );
    std::vector< int > krefOutput( length );
    std::vector< int > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0);
    std::fill(koutput.begin(),koutput.end(),0);
    std::fill(voutput.begin(),voutput.end(),0);

    bolt::cl::equal_to<int> binary_predictor;
    bolt::cl::plus<int> binary_operator;

    typedef std::pair<std::vector<int>::iterator,std::vector<int>::iterator> StdPairIterator;
    typedef bolt::cl::pair<std::vector<int>::iterator,std::vector<int>::iterator> DevicePairIterator;

    ::cl::Context myContext = bolt::cl::control::getDefault( ).getContext( );
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::SerialCpu);

    // call reduce_by_key
    DevicePairIterator dv_pair =
    bolt::cl::reduce_by_key(
        ctl,
        keys.begin(),
        keys.end(),
        input.begin(),
        koutput.begin(),
        voutput.begin(),
        binary_predictor,
        binary_operator);

#if 0

    for(unsigned int i = 0; i < length ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    DevicePairIterator gold_pair =
    gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(),
                        vrefOutput.begin(),std::plus<int>());

    size_t sizeAfterCall = gold_pair.first - krefOutput.begin();
    size_t sizeAfterDeviceCall = dv_pair.first - koutput.begin();

    std::cout<<sizeAfterCall<<" Is the gold key size after call!"<<std::endl;
    std::cout<<sizeAfterDeviceCall<<" Is the dv key size after call!"<<std::endl;

    krefOutput.resize(sizeAfterCall);
    vrefOutput.resize(sizeAfterCall);
    koutput.resize(sizeAfterDeviceCall);
    voutput.resize(sizeAfterDeviceCall);

#if 0

    for(unsigned int i = 0; i < sizeAfterDeviceCall ; i++)
    {
        std::cout<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
TEST(ReduceByKeyPairCheck, MultiCoreIntegerTest2)
{
    int length = 1<<10;
    std::vector< int > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    int key = 1;
    std::vector< int > refInput( length );
    std::vector< int > input( length );

    for (int i = 0; i < length; i++)
    {
        if(std::rand()%4 == 1) key++;
        keys[i] = key;
        refInput[i] = std::rand()%4;
        input[i] = refInput[i];
    }keys[1] = keys[2] = keys[3] = keys[4] =1;
    keys[5] = keys[6] = keys[7] = keys[8] =2; keys[9] = 3;

    // input and output vectors for device and reference

    std::vector< int > koutput( length );
    std::vector< int > voutput( length );
    std::vector< int > krefOutput( length );
    std::vector< int > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0);
    std::fill(koutput.begin(),koutput.end(),0);
    std::fill(voutput.begin(),voutput.end(),0);

    bolt::cl::equal_to<int> binary_predictor;
    bolt::cl::plus<int> binary_operator;

    typedef std::pair<std::vector<int>::iterator,std::vector<int>::iterator> StdPairIterator;
    typedef bolt::cl::pair<std::vector<int>::iterator,std::vector<int>::iterator> DevicePairIterator;

    ::cl::Context myContext = bolt::cl::control::getDefault( ).getContext( );
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    // call reduce_by_key
    DevicePairIterator dv_pair =
    bolt::cl::reduce_by_key(
        ctl,
        keys.begin(),
        keys.end(),
        input.begin(),
        koutput.begin(),
        voutput.begin(),
        binary_predictor,
        binary_operator);

#if 0

    for(unsigned int i = 0; i < length ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    DevicePairIterator gold_pair =
    gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(),
                        vrefOutput.begin(),std::plus<int>());

    size_t sizeAfterCall = gold_pair.first - krefOutput.begin();
    size_t sizeAfterDeviceCall = dv_pair.first - koutput.begin();

    std::cout<<sizeAfterCall<<" Is the gold key size after call!"<<std::endl;
    std::cout<<sizeAfterDeviceCall<<" Is the dv key size after call!"<<std::endl;

    krefOutput.resize(sizeAfterCall);
    vrefOutput.resize(sizeAfterCall);
    koutput.resize(sizeAfterDeviceCall);
    voutput.resize(sizeAfterDeviceCall);

#if 0

    for(unsigned int i = 0; i < sizeAfterDeviceCall ; i++)
    {
        std::cout<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}

TEST(ReduceByKeyPairCheck, FloatTest2)
{
    int length = 1<<15;
    std::vector< float > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    float key = 1.0f;
    std::vector< float > refInput( length );
    std::vector< float > input( length );

    for (int i = 0; i < length; i++)
    {
        if(std::rand()%3 == 1) key++;
        keys[i] = key;
        refInput[i] = (float)(std::rand()%4);
        input[i] = refInput[i];
    }keys[1] = keys[2] = keys[3] = keys[4] =1.0f;
    keys[5] = keys[6] = keys[7] = keys[8] =2.0f; keys[9] = 3.0f;

    // input and output vectors for device and reference

    std::vector< float > koutput( length );
    std::vector< float > voutput( length );
    std::vector< float > krefOutput( length );
    std::vector< float > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0.0f);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0.0f);
    std::fill(koutput.begin(),koutput.end(),0.0f);
    std::fill(voutput.begin(),voutput.end(),0.0f);

    bolt::cl::equal_to<float> binary_predictor;
    bolt::cl::plus<float> binary_operator;

    typedef std::pair<std::vector<float>::iterator,std::vector<float>::iterator> StdPairIterator;
    typedef bolt::cl::pair<std::vector<float>::iterator,std::vector<float>::iterator> DevicePairIterator;

    // call reduce_by_key
    DevicePairIterator dv_pair =
    bolt::cl::reduce_by_key(
        keys.begin(),
        keys.end(),
        input.begin(),
        koutput.begin(),
        voutput.begin(),
        binary_predictor,
        binary_operator);

#if 0

    for(unsigned int i = 0; i < length ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    DevicePairIterator gold_pair =
    gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(),
                        vrefOutput.begin(),std::plus<float>());

    size_t sizeAfterCall = gold_pair.first - krefOutput.begin();
    size_t sizeAfterDeviceCall = dv_pair.first - koutput.begin();

    std::cout<<sizeAfterCall<<" Is the gold key size after call!"<<std::endl;
    std::cout<<sizeAfterDeviceCall<<" Is the dv key size after call!"<<std::endl;

    krefOutput.resize(sizeAfterCall);
    vrefOutput.resize(sizeAfterCall);
    koutput.resize(sizeAfterDeviceCall);
    voutput.resize(sizeAfterDeviceCall);

#if 0

    for(unsigned int i = 0; i < sizeAfterDeviceCall ; i++)
    {
        std::cout<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
TEST(ReduceByKeyPairCheck, OffSetFloatTest2)
{
    int length = 1<<15;
    std::vector< float > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    float key = 1.0f;
    std::vector< float > refInput( length );
    std::vector< float > input( length );

    for (int i = 0; i < length; i++)
    {
        if(std::rand()%4 == 1) key++;
        keys[i] = key;
        refInput[i] = (float)(std::rand()%4);
        input[i] = refInput[i];
    }keys[1] = keys[2] = keys[3] = keys[4] =1.0f;
    keys[5] = keys[6] = keys[7] = keys[8] =2.0f; keys[9] = 3.0f;

    // input and output vectors for device and reference

    std::vector< float > koutput( length );
    std::vector< float > voutput( length );
    std::vector< float > krefOutput( length );
    std::vector< float > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0.0f);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0.0f);
    std::fill(koutput.begin(),koutput.end(),0.0f);
    std::fill(voutput.begin(),voutput.end(),0.0f);

    bolt::cl::equal_to<float> binary_predictor;
    bolt::cl::plus<float> binary_operator;

    typedef std::pair<std::vector<float>::iterator,std::vector<float>::iterator> StdPairIterator;
    typedef bolt::cl::pair<std::vector<float>::iterator,std::vector<float>::iterator> DevicePairIterator;

    // call reduce_by_key
    DevicePairIterator dv_pair =
    bolt::cl::reduce_by_key(
        keys.begin()+10,
        keys.begin()+400,
        input.begin()+10,
        koutput.begin()+10,
        voutput.begin()+10,
        binary_predictor,
        binary_operator);

#if 0

    for(unsigned int i = 0; i < length ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    DevicePairIterator gold_pair =
    gold_reduce_by_key( keys.begin()+10, keys.begin()+400, refInput.begin()+10, krefOutput.begin()+10,
                        vrefOutput.begin()+10,std::plus<float>());

    size_t sizeAfterCall = gold_pair.first - krefOutput.begin();
    size_t sizeAfterDeviceCall = dv_pair.first - koutput.begin();

    std::cout<<sizeAfterCall<<" Is the gold key size after call!"<<std::endl;
    std::cout<<sizeAfterDeviceCall<<" Is the dv key size after call!"<<std::endl;

    krefOutput.resize(sizeAfterCall);
    vrefOutput.resize(sizeAfterCall);
    koutput.resize(sizeAfterDeviceCall);
    voutput.resize(sizeAfterDeviceCall);

#if 0

    for(unsigned int i = 0; i < sizeAfterDeviceCall ; i++)
    {
        std::cout<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
TEST(ReduceByKeyPairCheck, SerialFloatTest2)
{
    int length = 1<<10;
    std::vector< float > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    float key = 1.0f;
    std::vector< float > refInput( length );
    std::vector< float > input( length );

    for (int i = 0; i < length; i++)
    {
        if(std::rand()%3 == 1) key++;
        keys[i] = key;
        refInput[i] = (float)(std::rand()%4);
        input[i] = refInput[i];
    }keys[1] = keys[2] = keys[3] = keys[4] =1.0f;
    keys[5] = keys[6] = keys[7] = keys[8] =2.0f; keys[9] = 3.0f;

    // input and output vectors for device and reference

    std::vector< float > koutput( length );
    std::vector< float > voutput( length );
    std::vector< float > krefOutput( length );
    std::vector< float > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0.0f);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0.0f);
    std::fill(koutput.begin(),koutput.end(),0.0f);
    std::fill(voutput.begin(),voutput.end(),0.0f);

    bolt::cl::equal_to<float> binary_predictor;
    bolt::cl::plus<float> binary_operator;

    typedef std::pair<std::vector<float>::iterator,std::vector<float>::iterator> StdPairIterator;
    typedef bolt::cl::pair<std::vector<float>::iterator,std::vector<float>::iterator> DevicePairIterator;

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::SerialCpu);

    // call reduce_by_key
    DevicePairIterator dv_pair =
    bolt::cl::reduce_by_key(
        ctl,
        keys.begin(),
        keys.end(),
        input.begin(),
        koutput.begin(),
        voutput.begin(),
        binary_predictor,
        binary_operator);

#if 0

    for(unsigned int i = 0; i < length ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    DevicePairIterator gold_pair =
    gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(),
                        vrefOutput.begin(),std::plus<float>());

    size_t sizeAfterCall = gold_pair.first - krefOutput.begin();
    size_t sizeAfterDeviceCall = dv_pair.first - koutput.begin();

    std::cout<<sizeAfterCall<<" Is the gold key size after call!"<<std::endl;
    std::cout<<sizeAfterDeviceCall<<" Is the dv key size after call!"<<std::endl;

    krefOutput.resize(sizeAfterCall);
    vrefOutput.resize(sizeAfterCall);
    koutput.resize(sizeAfterDeviceCall);
    voutput.resize(sizeAfterDeviceCall);

#if 0

    for(unsigned int i = 0; i < sizeAfterDeviceCall ; i++)
    {
        std::cout<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
TEST(ReduceByKeyPairCheck, MultiCoreFloatTest2)
{
    int length = 1<<10;
    std::vector< float > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    float key = 1.0f;
    std::vector< float > refInput( length );
    std::vector< float > input( length );

    for (int i = 0; i < length; i++)
    {
        if(std::rand()%3 == 1) key++;
        keys[i] = key;
        refInput[i] = (float)(std::rand()%4);
        input[i] = refInput[i];
    }keys[1] = keys[2] = keys[3] = keys[4] =1.0f;
    keys[5] = keys[6] = keys[7] = keys[8] =2.0f; keys[9] = 3.0f;

    // input and output vectors for device and reference

    std::vector< float > koutput( length );
    std::vector< float > voutput( length );
    std::vector< float > krefOutput( length );
    std::vector< float > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0.0f);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0.0f);
    std::fill(koutput.begin(),koutput.end(),0.0f);
    std::fill(voutput.begin(),voutput.end(),0.0f);

    bolt::cl::equal_to<float> binary_predictor;
    bolt::cl::plus<float> binary_operator;

    typedef std::pair<std::vector<float>::iterator,std::vector<float>::iterator> StdPairIterator;
    typedef bolt::cl::pair<std::vector<float>::iterator,std::vector<float>::iterator> DevicePairIterator;

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    // call reduce_by_key
    DevicePairIterator dv_pair =
    bolt::cl::reduce_by_key(
        ctl,
        keys.begin(),
        keys.end(),
        input.begin(),
        koutput.begin(),
        voutput.begin(),
        binary_predictor,
        binary_operator);

#if 0

    for(unsigned int i = 0; i < length ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    DevicePairIterator gold_pair =
    gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(),
                        vrefOutput.begin(),std::plus<float>());

    size_t sizeAfterCall = gold_pair.first - krefOutput.begin();
    size_t sizeAfterDeviceCall = dv_pair.first - koutput.begin();

    std::cout<<sizeAfterCall<<" Is the gold key size after call!"<<std::endl;
    std::cout<<sizeAfterDeviceCall<<" Is the dv key size after call!"<<std::endl;

    krefOutput.resize(sizeAfterCall);
    vrefOutput.resize(sizeAfterCall);
    koutput.resize(sizeAfterDeviceCall);
    voutput.resize(sizeAfterDeviceCall);

#if 0

    for(unsigned int i = 0; i < sizeAfterDeviceCall ; i++)
    {
        std::cout<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}

#if(TEST_DOUBLE == 1)
TEST(ReduceByKeyPairCheck, DoubleTest2)
{
    int length = 1<<24;
    std::vector< double > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    double key = 1.0;
    std::vector< double > refInput( length );
    std::vector< double > input( length );

    for (int i = 0; i < length; i++)
    {
        if(std::rand()%3 == 1) key++;
        keys[i] = key;
        refInput[i] = (double)(std::rand()%4);
        input[i] = refInput[i];
    }keys[1] = keys[2] = keys[3] = keys[4] =1.0;
    keys[5] = keys[6] = keys[7] = keys[8] =2.0; keys[9] = 3.0;

    // input and output vectors for device and reference

    std::vector< double > koutput( length );
    std::vector< double > voutput( length );
    std::vector< double > krefOutput( length );
    std::vector< double > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0.0);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0.0);
    std::fill(koutput.begin(),koutput.end(),0.0);
    std::fill(voutput.begin(),voutput.end(),0.0);

    bolt::cl::equal_to<double> binary_predictor;
    bolt::cl::plus<double> binary_operator;

    typedef std::pair<std::vector<double>::iterator,std::vector<double>::iterator> StdPairIterator;
    typedef bolt::cl::pair<std::vector<double>::iterator,std::vector<double>::iterator> DevicePairIterator;

    // call reduce_by_key
    DevicePairIterator dv_pair =
    bolt::cl::reduce_by_key(
        keys.begin(),
        keys.end(),
        input.begin(),
        koutput.begin(),
        voutput.begin(),
        binary_predictor,
        binary_operator);

#if 0

    for(unsigned int i = 0; i < length ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    DevicePairIterator gold_pair =
    gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(),
                        vrefOutput.begin(),std::plus<double>());

    size_t sizeAfterCall = gold_pair.first - krefOutput.begin();
    size_t sizeAfterDeviceCall = dv_pair.first - koutput.begin();

    std::cout<<sizeAfterCall<<" Is the gold key size after call!"<<std::endl;
    std::cout<<sizeAfterDeviceCall<<" Is the dv key size after call!"<<std::endl;

    krefOutput.resize(sizeAfterCall);
    vrefOutput.resize(sizeAfterCall);
    koutput.resize(sizeAfterDeviceCall);
    voutput.resize(sizeAfterDeviceCall);

#if 0

    for(unsigned int i = 0; i < sizeAfterDeviceCall ; i++)
    {
        std::cout<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}

TEST(ReduceByKeyPairCheck, OffSetDoubleTest2)
{
    int length = 1<<24;
    std::vector< double > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    double key = 1.0;
    std::vector< double > refInput( length );
    std::vector< double > input( length );

    for (int i = 0; i < length; i++)
    {
        if(std::rand()%3 == 1) key++;
        keys[i] = key;
        refInput[i] = (double)(std::rand()%4);
        input[i] = refInput[i];
    }keys[1] = keys[2] = keys[3] = keys[4] =1.0;
    keys[5] = keys[6] = keys[7] = keys[8] =2.0; keys[9] = 3.0;

    // input and output vectors for device and reference

    std::vector< double > koutput( length );
    std::vector< double > voutput( length );
    std::vector< double > krefOutput( length );
    std::vector< double > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0.0);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0.0);
    std::fill(koutput.begin(),koutput.end(),0.0);
    std::fill(voutput.begin(),voutput.end(),0.0);

    bolt::cl::equal_to<double> binary_predictor;
    bolt::cl::plus<double> binary_operator;

    typedef std::pair<std::vector<double>::iterator,std::vector<double>::iterator> StdPairIterator;
    typedef bolt::cl::pair<std::vector<double>::iterator,std::vector<double>::iterator> DevicePairIterator;

    // call reduce_by_key
    DevicePairIterator dv_pair =
    bolt::cl::reduce_by_key(
        keys.begin()+10,
        keys.begin()+300,
        input.begin()+10,
        koutput.begin()+10,
        voutput.begin()+10,
        binary_predictor,
        binary_operator);

#if 0

    for(unsigned int i = 0; i < length ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    DevicePairIterator gold_pair =
    gold_reduce_by_key( keys.begin()+10, keys.begin()+300, refInput.begin()+10, krefOutput.begin()+10,
                        vrefOutput.begin()+10,std::plus<double>());

    size_t sizeAfterCall = gold_pair.first - krefOutput.begin();
    size_t sizeAfterDeviceCall = dv_pair.first - koutput.begin();

    std::cout<<sizeAfterCall<<" Is the gold key size after call!"<<std::endl;
    std::cout<<sizeAfterDeviceCall<<" Is the dv key size after call!"<<std::endl;

    krefOutput.resize(sizeAfterCall);
    vrefOutput.resize(sizeAfterCall);
    koutput.resize(sizeAfterDeviceCall);
    voutput.resize(sizeAfterDeviceCall);

#if 0

    for(unsigned int i = 0; i < sizeAfterDeviceCall ; i++)
    {
        std::cout<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}

TEST(ReduceByKeyPairCheck, SerialDoubleTest2)
{
    int length = 1<<24;
    std::vector< double > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    double key = 1.0;
    std::vector< double > refInput( length );
    std::vector< double > input( length );

    for (int i = 0; i < length; i++)
    {
        if(std::rand()%4 == 1) key++;
        keys[i] = key;
        refInput[i] = (double)(std::rand()%4);
        input[i] = refInput[i];
    }keys[1] = keys[2] = keys[3] = keys[4] =1.0;
    keys[5] = keys[6] = keys[7] = keys[8] =2.0; keys[9] = 3.0;

    // input and output vectors for device and reference

    std::vector< double > koutput( length );
    std::vector< double > voutput( length );
    std::vector< double > krefOutput( length );
    std::vector< double > vrefOutput( length );
    std::fill(krefOutput.begin(),krefOutput.end(),0.0);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0.0);
    std::fill(koutput.begin(),koutput.end(),0.0);
    std::fill(voutput.begin(),voutput.end(),0.0);

    bolt::cl::equal_to<double> binary_predictor;
    bolt::cl::plus<double> binary_operator;

    typedef std::pair<std::vector<double>::iterator,std::vector<double>::iterator> StdPairIterator;
    typedef bolt::cl::pair<std::vector<double>::iterator,std::vector<double>::iterator> DevicePairIterator;

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::SerialCpu);

    // call reduce_by_key
    DevicePairIterator dv_pair =
    bolt::cl::reduce_by_key(
        ctl,
        keys.begin(),
        keys.end(),
        input.begin(),
        koutput.begin(),
        voutput.begin(),
        binary_predictor,
        binary_operator);

#if 0

    for(unsigned int i = 0; i < length ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    DevicePairIterator gold_pair =
    gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(),
                        vrefOutput.begin(),std::plus<double>());

    size_t sizeAfterCall = gold_pair.first - krefOutput.begin();
    size_t sizeAfterDeviceCall = dv_pair.first - koutput.begin();

    std::cout<<sizeAfterCall<<" Is the gold key size after call!"<<std::endl;
    std::cout<<sizeAfterDeviceCall<<" Is the dv key size after call!"<<std::endl;

    krefOutput.resize(sizeAfterCall);
    vrefOutput.resize(sizeAfterCall);
    koutput.resize(sizeAfterDeviceCall);
    voutput.resize(sizeAfterDeviceCall);

#if 0

    for(unsigned int i = 0; i < sizeAfterDeviceCall ; i++)
    {
        std::cout<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
TEST(ReduceByKeyPairCheck, MultiCoreDoubleTest2)
{
    int length = 1<<24;
    std::vector< double > keys( length);
    // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
    int segmentLength = 0;
    int segmentIndex = 0;
    double key = 1.0;
    std::vector< double > refInput( length );
    std::vector< double > input( length );

    for (int i = 0; i < length; i++)
    {
        if(std::rand()%3 == 1) key++;
        keys[i] = key;
        refInput[i] = (double)(std::rand()%4);
        input[i] = refInput[i];
    }keys[1] = keys[2] = keys[3] = keys[4] =1.0;
    keys[5] = keys[6] = keys[7] = keys[8] =2.0; keys[9] = 3.0;

    // input and output vectors for device and reference

    std::vector< double > koutput( length );
    std::vector< double > voutput( length );
    std::vector< double > krefOutput( length );
    std::vector< double > vrefOutput( length );

    std::fill(krefOutput.begin(),krefOutput.end(),0.0);
    std::fill(vrefOutput.begin(),vrefOutput.end(),0.0);
    std::fill(koutput.begin(),koutput.end(),0.0);
    std::fill(voutput.begin(),voutput.end(),0.0);

    bolt::cl::equal_to<double> binary_predictor;
    bolt::cl::plus<double> binary_operator;

    typedef std::pair<std::vector<double>::iterator,std::vector<double>::iterator> StdPairIterator;
    typedef bolt::cl::pair<std::vector<double>::iterator,std::vector<double>::iterator> DevicePairIterator;

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    // call reduce_by_key
    DevicePairIterator dv_pair =
    bolt::cl::reduce_by_key(
        ctl,
        keys.begin(),
        keys.end(),
        input.begin(),
        koutput.begin(),
        voutput.begin(),
        binary_predictor,
        binary_operator);

#if 0

    for(unsigned int i = 0; i < length ; i++)
    {
      std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    DevicePairIterator gold_pair =
    gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(),
                        vrefOutput.begin(),std::plus<double>());

    size_t sizeAfterCall = gold_pair.first - krefOutput.begin();
    size_t sizeAfterDeviceCall = dv_pair.first - koutput.begin();

    std::cout<<sizeAfterCall<<" Is the gold key size after call!"<<std::endl;
    std::cout<<sizeAfterDeviceCall<<" Is the dv key size after call!"<<std::endl;

    krefOutput.resize(sizeAfterCall);
    vrefOutput.resize(sizeAfterCall);
    koutput.resize(sizeAfterDeviceCall);
    voutput.resize(sizeAfterDeviceCall);

#if 0

    for(unsigned int i = 0; i < sizeAfterDeviceCall ; i++)
    {
        std::cout<<" -> OKeys "<<koutput[i]<<" OValues "<<voutput[i]<<std::endl;
    }

#endif

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);
}
#endif
TEST(ReduceByKeyBasic, IntegerTestOddSizes)
{
    int length;

    int num,i, count=0;

   for(num=1<<10;count<=20;num++)
   {
      for(i=2;i<num;i++)
      {
         if(num%i==0)
         break;
      }
      if(num==i)
      {
      //  printf("****%d\n", length);
        length = num;
        std::vector< int > keys( length);
        // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
        int segmentLength = 0;
        int segmentIndex = 0;
        int key = 1;
        std::vector< int > refInput( length );
        std::vector< int > input( length );
        for (int i = 0; i < length; i++)
        {
            if(std::rand()%3 == 1) key++;
            keys[i] = key;
            refInput[i] = std::rand()%4;
            input[i] = refInput[i];
        }

        // input and output vectors for device and reference

        std::vector< int > koutput( length );
        std::vector< int > voutput( length );
        std::vector< int > krefOutput( length );
        std::vector< int > vrefOutput( length );
        std::fill(krefOutput.begin(),krefOutput.end(),0);
        std::fill(vrefOutput.begin(),vrefOutput.end(),0);


        bolt::cl::equal_to<int> binary_predictor;
        bolt::cl::plus<int> binary_operator;


        // call reduce_by_key

        auto p = bolt::cl::reduce_by_key( keys.begin(), keys.end(), input.begin(), koutput.begin(), voutput.begin(),
                                          binary_predictor, binary_operator);


    #if 0

        for(unsigned int i = 0; i < 256 ; i++)
        {

            std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i];
            std::cout<<" OValues "<<voutput[i]<<std::endl;

        }

    #endif


        auto refPair = gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(),
                                           vrefOutput.begin(),std::plus<int>());


        //cmpArrays2(krefOutput, koutput, refPair.first, p.first);
        cmpArrays(krefOutput, koutput);
        cmpArrays(vrefOutput, voutput);
        // cmpArrays2(vrefOutput, voutput, refPair.second, p.second);
        count++;
      }
   }


}
TEST(ReduceByKeyBasic, OffSetIntegerTestOddSizes)
{
    int length;
    int num,i, count=0;
   for(num=1<<10;count<=20;num++)
   {
      for(i=2;i<num;i++)
      {
         if(num%i==0)
         break;
      }
      if(num==i)
      {
      //  printf("****%d\n", length);
        length = num;
        std::vector< int > keys( length);
        // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
        int segmentLength = 0;
        int segmentIndex = 0;
        int key = 1;
        std::vector< int > refInput( length );
        std::vector< int > input( length );
        for (int i = 0; i < length; i++)
        {
            if(std::rand()%3 == 1) key++;
            keys[i] = key;
            refInput[i] = std::rand()%4;
            input[i] = refInput[i];
        }
        // input and output vectors for device and reference

        std::vector< int > koutput( length );
        std::vector< int > voutput( length );
        std::vector< int > krefOutput( length );
        std::vector< int > vrefOutput( length );
        std::fill(krefOutput.begin(),krefOutput.end(),0);
        std::fill(vrefOutput.begin(),vrefOutput.end(),0);

        bolt::cl::equal_to<int> binary_predictor;
        bolt::cl::plus<int> binary_operator;

        // call reduce_by_key
        auto p = bolt::cl::reduce_by_key( keys.begin()+10, keys.begin()+450, input.begin()+10, koutput.begin()+10, voutput.begin()+10,
                                          binary_predictor, binary_operator);
    #if 0

        for(unsigned int i = 0; i < 256 ; i++)
        {

            std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i];
            std::cout<<" OValues "<<voutput[i]<<std::endl;

        }

    #endif


        auto refPair = gold_reduce_by_key( keys.begin()+10, keys.begin()+450, refInput.begin()+10, krefOutput.begin()+10,
                                           vrefOutput.begin()+10,std::plus<int>());


        //cmpArrays2(krefOutput, koutput, refPair.first, p.first);
        cmpArrays(krefOutput, koutput);
        cmpArrays(vrefOutput, voutput);
        // cmpArrays2(vrefOutput, voutput, refPair.second, p.second);
        count++;
      }
   }


}
TEST(ReduceByKeyBasic, SerialIntegerTestOddSizes)
{
    int length;

    int num,i, count=0;

   for(num=1<<10;count<=20;num++)
   {
      for(i=2;i<num;i++)
      {
         if(num%i==0)
         break;
      }
      if(num==i)
      {
      //  printf("****%d\n", length);
        length = num;
        std::vector< int > keys( length);
        // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
        int segmentLength = 0;
        int segmentIndex = 0;
        int key = 1;
        std::vector< int > refInput( length );
        std::vector< int > input( length );
        for (int i = 0; i < length; i++)
        {
            if(std::rand()%3 == 1) key++;
            keys[i] = key;
            refInput[i] = std::rand()%4;
            input[i] = refInput[i];
        }

        // input and output vectors for device and reference

        std::vector< int > koutput( length );
        std::vector< int > voutput( length );
        std::vector< int > krefOutput( length );
        std::vector< int > vrefOutput( length );
        std::fill(krefOutput.begin(),krefOutput.end(),0);
        std::fill(vrefOutput.begin(),vrefOutput.end(),0);


        bolt::cl::equal_to<int> binary_predictor;
        bolt::cl::plus<int> binary_operator;

        ::cl::Context myContext = bolt::cl::control::getDefault( ).getContext( );
        bolt::cl::control ctl = bolt::cl::control::getDefault( );
        ctl.setForceRunMode(bolt::cl::control::SerialCpu);

        // call reduce_by_key
        auto p = bolt::cl::reduce_by_key( ctl, keys.begin(), keys.end(), input.begin(), koutput.begin(),
                                          voutput.begin(), binary_predictor, binary_operator);

    #if 0

        for(unsigned int i = 0; i < 256 ; i++)
        {
             std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i];
             std::cout<<" OValues "<<voutput[i]<<std::endl;
        }

    #endif

        auto refPair = gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(),
                                           vrefOutput.begin(),std::plus<int>());

        //cmpArrays2(krefOutput, koutput, refPair.first, p.first);
        cmpArrays(krefOutput, koutput);
        cmpArrays(vrefOutput, voutput);
        // cmpArrays2(vrefOutput, voutput, refPair.second, p.second);
        count++;
      }
   }


}
TEST(ReduceByKeyBasic, MultiCoreIntegerTestOddSizes)
{	
    int length;
    int num,i, count=0;
   for(num=1<<10;count<=20;num++)
   {
      for(i=2;i<num;i++)
      {
         if(num%i==0)
         break;
      }
      if(num==i)
      {
      //  printf("****%d\n", length);
        length = num;
        std::vector< int > keys( length);
        // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
        int segmentLength = 0;
        int segmentIndex = 0;
        int key = 1;
        std::vector< int > refInput( length );
        std::vector< int > input( length );
        for (int i = 0; i < length; i++)
        {
            if(std::rand()%3 == 1) key++;
            keys[i] = key;
            refInput[i] = std::rand()%4;
            input[i] = refInput[i];
        }
        // input and output vectors for device and reference
        std::vector< int > koutput( length );
        std::vector< int > voutput( length );
        std::vector< int > krefOutput( length );
        std::vector< int > vrefOutput( length );
        std::fill(krefOutput.begin(),krefOutput.end(),0);
        std::fill(vrefOutput.begin(),vrefOutput.end(),0);

        bolt::cl::equal_to<int> binary_predictor;
        bolt::cl::plus<int> binary_operator;

        bolt::cl::control ctl = bolt::cl::control::getDefault( );
        ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);
for(int a=0; a<100; a++)
{
       // call reduce_by_key
        auto p = bolt::cl::reduce_by_key( ctl, keys.begin(), keys.end(), input.begin(), koutput.begin(),
                                          voutput.begin(), binary_predictor, binary_operator);
    #if 0
        for(unsigned int i = 0; i < 256 ; i++)
        {
            std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i];
            std::cout<<" OValues "<<voutput[i]<<std::endl;
        }
    #endif
        auto refPair = gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(),
                                           vrefOutput.begin(),std::plus<int>());
        cmpArrays(krefOutput, koutput);
        cmpArrays(vrefOutput, voutput);
}       
        count++;
      }
   }
} 

TEST(ReduceByKeyBasic, FloatTestOddSizes)
{
    int length;
    int num,i, count=0;
   for(num=1<<10;count<=20;num++)
   {
      for(i=2;i<num;i++)
      {
         if(num%i==0)
         break;
      }
      if(num==i)
      {
      //  printf("****%d\n", length);
        length = num;
        std::vector< float > keys( length);
        // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
        int segmentLength = 0;
        int segmentIndex = 0;
        float key = 1.0f;
        std::vector< float > refInput( length );
        std::vector< float > input( length );
        for (int i = 0; i < length; i++)
        {
            if(std::rand()%3 == 1) key++;
            keys[i] = key;
            refInput[i] = (float)(std::rand()%4);
            input[i] = refInput[i];
        }

        // input and output vectors for device and reference

        std::vector< float > koutput( length );
        std::vector< float > voutput( length );
        std::vector< float > krefOutput( length );
        std::vector< float > vrefOutput( length );
        std::fill(krefOutput.begin(),krefOutput.end(),0.0f);
        std::fill(vrefOutput.begin(),vrefOutput.end(),0.0f);


        bolt::cl::equal_to<float> binary_predictor;
        bolt::cl::plus<float> binary_operator;


        // call reduce_by_key

        auto p = bolt::cl::reduce_by_key( keys.begin(), keys.end(), input.begin(), koutput.begin(), voutput.begin(),
                                          binary_predictor, binary_operator);


    #if 0
        for(unsigned int i = 0; i < 256 ; i++)
        {
            std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i];
            std::cout<<" OValues "<<voutput[i]<<std::endl;
        }
    #endif
        auto refPair = gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(),
                                           vrefOutput.begin(),std::plus<float>());
        cmpArrays(krefOutput, koutput);
        cmpArrays(vrefOutput, voutput);
        count++;
      }
   }
}
TEST(ReduceByKeyBasic, OffSetFloatTestOddSizes)
{
    int length;
    int num,i, count=0;
   for(num=1<<10;count<=20;num++)
   {
      for(i=2;i<num;i++)
      {
         if(num%i==0)
         break;
      }
      if(num==i)
      {
      //  printf("****%d\n", length);
        length = num;
        std::vector< float > keys( length);
        // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
        int segmentLength = 0;
        int segmentIndex = 0;
        float key = 1.0f;
        std::vector< float > refInput( length );
        std::vector< float > input( length );
        for (int i = 0; i < length; i++)
        {
            if(std::rand()%3 == 1) key++;
            keys[i] = key;
            refInput[i] = (float)(std::rand()%4);
            input[i] = refInput[i];
        }

        // input and output vectors for device and reference

        std::vector< float > koutput( length );
        std::vector< float > voutput( length );
        std::vector< float > krefOutput( length );
        std::vector< float > vrefOutput( length );
        std::fill(krefOutput.begin(),krefOutput.end(),0.0f);
        std::fill(vrefOutput.begin(),vrefOutput.end(),0.0f);


        bolt::cl::equal_to<float> binary_predictor;
        bolt::cl::plus<float> binary_operator;


        // call reduce_by_key

        auto p = bolt::cl::reduce_by_key( keys.begin()+10, keys.begin()+400, input.begin()+10, koutput.begin()+10,
			                             voutput.begin()+10,binary_predictor, binary_operator);


    #if 0
        for(unsigned int i = 0; i < 256 ; i++)
        {
            std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i];
            std::cout<<" OValues "<<voutput[i]<<std::endl;
        }
    #endif
        auto refPair = gold_reduce_by_key( keys.begin()+10, keys.begin()+400, refInput.begin()+10, krefOutput.begin()+10,
                                           vrefOutput.begin()+10,std::plus<float>());
        cmpArrays(krefOutput, koutput);
        cmpArrays(vrefOutput, voutput);
        count++;
      }
   }
}
TEST(ReduceByKeyBasic, SerialFloatTestOddSizes)
{
    int length;
    int num,i, count=0;
   for(num=1<<10;count<=20;num++)
   {
      for(i=2;i<num;i++)
      {
         if(num%i==0)
         break;
      }
      if(num==i)
      {
      //  printf("****%d\n", length);
        length = num;
        std::vector< float > keys( length);
        // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
        int segmentLength = 0;
        int segmentIndex = 0;
        float key = 1.0f;
        std::vector< float > refInput( length );
        std::vector< float > input( length );
        for (int i = 0; i < length; i++)
        {
            if(std::rand()%3 == 1) key++;
            keys[i] = key;
            refInput[i] = (float)(std::rand()%4);
            input[i] = refInput[i];
        }
        // input and output vectors for device and reference

        std::vector< float > koutput( length );
        std::vector< float > voutput( length );
        std::vector< float > krefOutput( length );
        std::vector< float > vrefOutput( length );
        std::fill(krefOutput.begin(),krefOutput.end(),0.0f);
        std::fill(vrefOutput.begin(),vrefOutput.end(),0.0f);
        
        bolt::cl::equal_to<float> binary_predictor;
        bolt::cl::plus<float> binary_operator;

        bolt::cl::control ctl = bolt::cl::control::getDefault( );
        ctl.setForceRunMode(bolt::cl::control::SerialCpu);

        // call reduce_by_key
        auto p = bolt::cl::reduce_by_key( ctl, keys.begin(), keys.end(), input.begin(), koutput.begin(),
                                          voutput.begin(), binary_predictor, binary_operator);
#if 0
        for(unsigned int i = 0; i < 256 ; i++)
        {
             std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i];
             std::cout<<" OValues "<<voutput[i]<<std::endl;
        }
    #endif
        auto refPair = gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(),
                                           vrefOutput.begin(),std::plus<float>());
        cmpArrays(krefOutput, koutput);
        cmpArrays(vrefOutput, voutput);
        count++;
      }
   }
}
TEST(ReduceByKeyBasic, MultiCoreFloatTestOddSizes)
{	
    int length;
    int num,i, count=0;
   for(num=1<<10;count<=20;num++)
   {
      for(i=2;i<num;i++)
      {
         if(num%i==0)
         break;
      }
      if(num==i)
      {
      //  printf("****%d\n", length);
        length = num;
        std::vector< float > keys( length);
        // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
        int segmentLength = 0;
        int segmentIndex = 0;
        float key = 1.0f;
        std::vector< float > refInput( length );
        std::vector< float > input( length );
        for (int i = 0; i < length; i++)
        {
            if(std::rand()%3 == 1) key++;
            keys[i] = key;
            refInput[i] = (float)(std::rand()%4);
            input[i] = refInput[i];
        }
        // input and output vectors for device and reference
        std::vector< float > koutput( length );
        std::vector< float > voutput( length );
        std::vector< float > krefOutput( length );
        std::vector< float > vrefOutput( length );
        std::fill(krefOutput.begin(),krefOutput.end(),0.0f);
        std::fill(vrefOutput.begin(),vrefOutput.end(),0.0f);

        bolt::cl::equal_to<float> binary_predictor;
        bolt::cl::plus<float> binary_operator;

        bolt::cl::control ctl = bolt::cl::control::getDefault( );
        ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

for(int a=0; a<100; a++)
{
       // call reduce_by_key
        auto p = bolt::cl::reduce_by_key( ctl, keys.begin(), keys.end(), input.begin(), koutput.begin(),
                                          voutput.begin(), binary_predictor, binary_operator);
   
        auto refPair = gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(),
                                           vrefOutput.begin(),std::plus<float>());
        cmpArrays(krefOutput, koutput);
        cmpArrays(vrefOutput, voutput);
}       
        count++;
      }
   }
} 

#if(TEST_DOUBLE==1)

TEST(ReduceByKeyBasic, DoubleTestOddSizes)
{
    int length;
    int num,i, count=0;
   for(num=1<<16;count<=20;num++)
   {
      for(i=2;i<num;i++)
      {
         if(num%i==0)
         break;
      }
      if(num==i)
      {
      //  printf("****%d\n", length);
        length = num;
        std::vector< double > keys( length);
        // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
        int segmentLength = 0;
        int segmentIndex = 0;
        double key = 1.0;
        std::vector< double > refInput( length );
        std::vector< double > input( length );
        for (int i = 0; i < length; i++)
        {
            if(std::rand()%3 == 1) key++;
            keys[i] = key;
            refInput[i] = (double)(std::rand()%4);
            input[i] = refInput[i];
        }

        // input and output vectors for device and reference

        std::vector< double > koutput( length );
        std::vector< double > voutput( length );
        std::vector< double > krefOutput( length );
        std::vector< double > vrefOutput( length );
        std::fill(krefOutput.begin(),krefOutput.end(),0.0);
        std::fill(vrefOutput.begin(),vrefOutput.end(),0.0);


        bolt::cl::equal_to<double> binary_predictor;
        bolt::cl::plus<double> binary_operator;


        // call reduce_by_key
        auto p = bolt::cl::reduce_by_key( keys.begin(), keys.end(), input.begin(), koutput.begin(), voutput.begin(),
                                          binary_predictor, binary_operator);
    #if 0
        for(unsigned int i = 0; i < 256 ; i++)
        {
            std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i];
            std::cout<<" OValues "<<voutput[i]<<std::endl;
        }
    #endif
        auto refPair = gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(),
                                           vrefOutput.begin(),std::plus<double>());
        cmpArrays(krefOutput, koutput);
        cmpArrays(vrefOutput, voutput);
        count++;
      }
   }
}
TEST(ReduceByKeyBasic, OffSetDoubleTestOddSizes)
{
    int length;
    int num,i, count=0;
   for(num=1<<16;count<=20;num++)
   {
      for(i=2;i<num;i++)
      {
         if(num%i==0)
         break;
      }
      if(num==i)
      {
        length = num;
        std::vector< double > keys( length);
        // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
        int segmentLength = 0;
        int segmentIndex = 0;
        double key = 1.0;
        std::vector< double > refInput( length );
        std::vector< double > input( length );
        for (int i = 0; i < length; i++)
        {
            if(std::rand()%3 == 1) key++;
            keys[i] = key;
            refInput[i] = (double)(std::rand()%4);
            input[i] = refInput[i];
        }

        // input and output vectors for device and reference

        std::vector< double > koutput( length );
        std::vector< double > voutput( length );
        std::vector< double > krefOutput( length );
        std::vector< double > vrefOutput( length );
        std::fill(krefOutput.begin(),krefOutput.end(),0.0);
        std::fill(vrefOutput.begin(),vrefOutput.end(),0.0);
		std::fill(koutput.begin(),koutput.end(),0.0);
        std::fill(voutput.begin(),voutput.end(),0.0);


        bolt::cl::equal_to<double> binary_predictor;
        bolt::cl::plus<double> binary_operator;


        // call reduce_by_key

        auto p = bolt::cl::reduce_by_key( keys.begin()+10, keys.begin()+500, input.begin()+10, koutput.begin()+10, voutput.begin()+10,
                                          binary_predictor, binary_operator);


    #if 0
        for(unsigned int i = 0; i < 256 ; i++)
        {
            std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i];
            std::cout<<" OValues "<<voutput[i]<<std::endl;
        }
    #endif
        auto refPair = gold_reduce_by_key( keys.begin()+10, keys.begin()+500, refInput.begin()+10, krefOutput.begin()+10,
                                           vrefOutput.begin()+10,std::plus<double>());
        cmpArrays(krefOutput, koutput);
        cmpArrays(vrefOutput, voutput);
        count++;
      }
   }
}

TEST(ReduceByKeyBasic, SerialDoubleTestOddSizes)
{
    int length;
    int num,i, count=0;
   for(num=1<<16;count<=20;num++)
   {
      for(i=2;i<num;i++)
      {
         if(num%i==0)
         break;
      }
      if(num==i)
      {
      //  printf("****%d\n", length);
        length = num;
        std::vector< double > keys( length);
        // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
        int segmentLength = 0;
        int segmentIndex = 0;
        double key = 1.0;
        std::vector< double > refInput( length );
        std::vector< double > input( length );
        for (int i = 0; i < length; i++)
        {
            if(std::rand()%3 == 1) key++;
            keys[i] = key;
            refInput[i] = (double)(std::rand()%4);
            input[i] = refInput[i];
        }
        // input and output vectors for device and reference

        std::vector< double > koutput( length );
        std::vector< double > voutput( length );
        std::vector< double > krefOutput( length );
        std::vector< double > vrefOutput( length );
        std::fill(krefOutput.begin(),krefOutput.end(),0.0);
        std::fill(vrefOutput.begin(),vrefOutput.end(),0.0);
        
        bolt::cl::equal_to<double> binary_predictor;
        bolt::cl::plus<double> binary_operator;

        bolt::cl::control ctl = bolt::cl::control::getDefault( );
        ctl.setForceRunMode(bolt::cl::control::SerialCpu);

        // call reduce_by_key
        auto p = bolt::cl::reduce_by_key( ctl, keys.begin(), keys.end(), input.begin(), koutput.begin(),
                                          voutput.begin(), binary_predictor, binary_operator);
#if 0
        for(unsigned int i = 0; i < 256 ; i++)
        {
             std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i];
             std::cout<<" OValues "<<voutput[i]<<std::endl;
        }
    #endif
        auto refPair = gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(),
                                           vrefOutput.begin(),std::plus<double>());
        cmpArrays(krefOutput, koutput);
        cmpArrays(vrefOutput, voutput);
        count++;
      }
   }
}
TEST(ReduceByKeyBasic, MultiCoreDoubleTestOddSizes)
{	
    int length;
    int num,i, count=0;
   for(num=1<<8;count<=20;num++)
   {
      for(i=2;i<num;i++)
      {
         if(num%i==0)
         break;
      }
      if(num==i)
      {
      //  printf("****%d\n", length);
        length = num;
        std::vector< double > keys( length);
        // keys = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5,...}
        int segmentLength = 0;
        int segmentIndex = 0;
        double key = 1.0;
        std::vector< double > refInput( length );
        std::vector< double > input( length );
        for (int i = 0; i < length; i++)
        {
            if(std::rand()%3 == 1) key++;
            keys[i] = key;
            refInput[i] = (double)(std::rand()%4);
            input[i] = refInput[i];
        }
        // input and output vectors for device and reference
        std::vector< double > koutput( length );
        std::vector< double > voutput( length );
        std::vector< double > krefOutput( length );
        std::vector< double > vrefOutput( length );
        std::fill(krefOutput.begin(),krefOutput.end(),0.0f);
        std::fill(vrefOutput.begin(),vrefOutput.end(),0.0f);

        bolt::cl::equal_to<double> binary_predictor;
        bolt::cl::plus<double> binary_operator;

        bolt::cl::control ctl = bolt::cl::control::getDefault( );
        ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);
for(int a=0; a<100; a++)
{
       // call reduce_by_key
        auto p = bolt::cl::reduce_by_key( ctl, keys.begin(), keys.end(), input.begin(), koutput.begin(),
                                          voutput.begin(), binary_predictor, binary_operator);
    #if 0
        for(unsigned int i = 0; i < 256 ; i++)
        {
            std::cout<<"Ikey "<<keys[i]<<" IValues "<<input[i]<<" -> OKeys "<<koutput[i];
            std::cout<<" OValues "<<voutput[i]<<std::endl;
        }
    #endif
        auto refPair = gold_reduce_by_key( keys.begin(), keys.end(), refInput.begin(), krefOutput.begin(),
                                           vrefOutput.begin(),std::plus<double>());
        cmpArrays(krefOutput, koutput);
        cmpArrays(vrefOutput, voutput);
}       
        count++;
      }
   }
}
#endif

BOLT_FUNCTOR(uddfltint,
struct uddfltint
{
    float x;
    int y;

    bool operator==(const uddfltint& rhs) const
    {
        bool equal = true;
        float ths = 0.00001f; // thresh hold single(float)
        equal = ( x == rhs.x ) ? equal : false;
        if (rhs.y < ths && rhs.y > -ths)
            equal = ( (1.0*y - rhs.y) < ths && (1.0*y - rhs.y) > -ths) ? equal : false;
        else
            equal = ( (1.0*y - rhs.y)/rhs.y < ths && (1.0*y - rhs.y)/rhs.y > -ths) ? equal : false;
        return equal;
    }
    void operator++(int)
    {
        x += (float)1.0;
        y += 1;
    }

    bool operator>(int rhs) const
    {

      bool greater = true;
      greater = ( x > rhs ) ? greater : false;
      greater = ( y > rhs ) ? greater : false;
      return greater;

    }
    void operator=(int rhs)
    {
      x = (float)rhs;
      y = rhs;
    }

    bool operator!=(int rhs) const
    {
        bool nequal = true;
        float ths = 0.00001f; // thresh hold single(float)
        nequal = ( x != rhs ) ? nequal : false;
        if (rhs < ths && rhs > -ths)
            nequal = ( (1.0*y - rhs) < ths && (1.0*y - rhs) > -ths) ? false : nequal;
        else
            nequal = ( (1.0*y - rhs)/rhs < ths && (1.0*y - rhs)/rhs > -ths) ? false : nequal;
        return nequal;
    }

    void operator-(int rhs)
    {
        x -= (float)rhs;
        y -= rhs;
    }
};
);

BOLT_FUNCTOR(uddfltint_equal_to,
struct uddfltint_equal_to
{
    bool operator()(const uddfltint& lhs, const uddfltint& rhs) const
    {
        return lhs == rhs;
    };
};);

BOLT_FUNCTOR(uddfltint_plus,
struct uddfltint_plus
{
    uddfltint operator()(const uddfltint &lhs, const uddfltint &rhs) const
    {
        uddfltint _result;
        _result.x = lhs.x+rhs.x;
        _result.y = lhs.y+rhs.y;
        return _result;
    }
};
);
BOLT_CREATE_TYPENAME( bolt::cl::device_vector< uddfltint >::iterator );
BOLT_CREATE_CLCODE( bolt::cl::device_vector< uddfltint >::iterator, bolt::cl::deviceVectorIteratorTemplate );

#if UDD
TEST(ReduceByKeyPairUDDTest, UDDFloatIntTest)
{
    int length = 1024;
    std::vector< uddfltint > keys( length);
    uddfltint key;
    key.x = 1.0f;
    key.y = 1;
    std::vector< uddfltint > refInput( length );
    std::vector< uddfltint > input( length );

    for (int i = 0; i < length; i++)
    {
        if(std::rand()%5 == 1) key++;
        keys[i] = key;
        refInput[i].x = float(std::rand()%4);
        refInput[i].y = std::rand()%4;
        input[i] = refInput[i];
    }

    std::vector< uddfltint > koutput( length );
    std::vector< uddfltint > voutput( length );
    std::vector< uddfltint > krefOutput( length );
    std::vector< uddfltint > vrefOutput( length );

    // Instead of using fill
    krefOutput.clear();krefOutput.resize(length);
    vrefOutput.clear();vrefOutput.resize(length);
    voutput.clear();voutput.resize(length);
    koutput.clear();koutput.resize(length);

    uddfltint_equal_to binary_predictor;
    uddfltint_plus binary_operator;

    typedef std::pair<std::vector<uddfltint>::iterator,
            std::vector<uddfltint>::iterator> StdPairIterator;
    typedef bolt::cl::pair<std::vector<uddfltint>::iterator,
            std::vector<uddfltint>::iterator> DevicePairIterator;

    DevicePairIterator gold_pair =
    gold_reduce_by_key( keys.begin(),
                        keys.end(),
                        refInput.begin(),
                        krefOutput.begin(),
                        vrefOutput.begin(),
                        binary_operator);

    // call reduce_by_key
    DevicePairIterator dv_pair =
    bolt::cl::reduce_by_key(
        keys.begin(),
        keys.end(),
        input.begin(),
        koutput.begin(),
        voutput.begin(),
        binary_predictor,
        binary_operator);

    size_t sizeAfterCall = gold_pair.first - krefOutput.begin();
    size_t sizeAfterDeviceCall = dv_pair.first - koutput.begin();

    krefOutput.resize(sizeAfterCall);
    vrefOutput.resize(sizeAfterCall);
    koutput.resize(sizeAfterDeviceCall);
    voutput.resize(sizeAfterDeviceCall);

#if 0

    for(unsigned int i = 0; i < sizeAfterDeviceCall ; i++)
    {
        std::cout<<" -> OKeys "<<koutput[i].x<<" OValues "<<voutput[i].x<<std::endl;
    }

#endif

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);

}
TEST(ReduceByKeyPairUDDTest, OffSetUDDFloatIntTest)
{
    int length = 1024;
    std::vector< uddfltint > keys( length);
    uddfltint key;
    key.x = 1.0f;
    key.y = 1;
    std::vector< uddfltint > refInput( length );
    std::vector< uddfltint > input( length );

    for (int i = 0; i < length; i++)
    {
        if(std::rand()%5 == 1) key++;
        keys[i] = key;
        refInput[i].x = float(std::rand()%4);
        refInput[i].y = std::rand()%4;
        input[i] = refInput[i];
    }

    std::vector< uddfltint > koutput( length );
    std::vector< uddfltint > voutput( length );
    std::vector< uddfltint > krefOutput( length );
    std::vector< uddfltint > vrefOutput( length );

    // Instead of using fill
    krefOutput.clear();krefOutput.resize(length);
    vrefOutput.clear();vrefOutput.resize(length);
    voutput.clear();voutput.resize(length);
    koutput.clear();koutput.resize(length);

    uddfltint_equal_to binary_predictor;
    uddfltint_plus binary_operator;

    typedef std::pair<std::vector<uddfltint>::iterator,
            std::vector<uddfltint>::iterator> StdPairIterator;
    typedef bolt::cl::pair<std::vector<uddfltint>::iterator,
            std::vector<uddfltint>::iterator> DevicePairIterator;

    DevicePairIterator gold_pair =
    gold_reduce_by_key( keys.begin()+100,
                        keys.begin()+400,
                        refInput.begin()+100,
                        krefOutput.begin()+100,
                        vrefOutput.begin()+100,
                        binary_operator);

    // call reduce_by_key
    DevicePairIterator dv_pair =
    bolt::cl::reduce_by_key(
        keys.begin()+100,
        keys.begin()+400,
        input.begin()+100,
        koutput.begin()+100,
        voutput.begin()+100,
        binary_predictor,
        binary_operator);

    size_t sizeAfterCall = gold_pair.first - krefOutput.begin();
    size_t sizeAfterDeviceCall = dv_pair.first - koutput.begin();

    krefOutput.resize(sizeAfterCall);
    vrefOutput.resize(sizeAfterCall);
    koutput.resize(sizeAfterDeviceCall);
    voutput.resize(sizeAfterDeviceCall);

#if 0

    for(unsigned int i = 0; i < sizeAfterDeviceCall ; i++)
    {
        std::cout<<" -> OKeys "<<koutput[i].x<<" OValues "<<voutput[i].x<<std::endl;
    }

#endif

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);

}
TEST(ReduceByKeyPairUDDTest, SerialUDDFloatIntTest)
{
    int length = 1024;
    std::vector< uddfltint > keys( length);
    uddfltint key;
    key.x = 1.0f;
    key.y = 1;
    std::vector< uddfltint > refInput( length );
    std::vector< uddfltint > input( length );

    for (int i = 0; i < length; i++)
    {
        if(std::rand()%5 == 1) key++;
        keys[i] = key;
        refInput[i].x = float(std::rand()%4);
        refInput[i].y = std::rand()%4;
        input[i] = refInput[i];
    }

    std::vector< uddfltint > koutput( length );
    std::vector< uddfltint > voutput( length );
    std::vector< uddfltint > krefOutput( length );
    std::vector< uddfltint > vrefOutput( length );

    // Instead of using fill
    krefOutput.clear();krefOutput.resize(length);
    vrefOutput.clear();vrefOutput.resize(length);
    voutput.clear();voutput.resize(length);
    koutput.clear();koutput.resize(length);

    uddfltint_equal_to binary_predictor;
    uddfltint_plus binary_operator;

    typedef std::pair<std::vector<uddfltint>::iterator,
            std::vector<uddfltint>::iterator> StdPairIterator;
    typedef bolt::cl::pair<std::vector<uddfltint>::iterator,
            std::vector<uddfltint>::iterator> DevicePairIterator;

    DevicePairIterator gold_pair =
    gold_reduce_by_key( keys.begin(),
                        keys.end(),
                        refInput.begin(),
                        krefOutput.begin(),
                        vrefOutput.begin(),
                        binary_operator);

    ::cl::Context myContext = bolt::cl::control::getDefault( ).getContext( );
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::SerialCpu);

    // call reduce_by_key
    DevicePairIterator dv_pair =
    bolt::cl::reduce_by_key(
        ctl,
        keys.begin(),
        keys.end(),
        input.begin(),
        koutput.begin(),
        voutput.begin(),
        binary_predictor,
        binary_operator);

    size_t sizeAfterCall = gold_pair.first - krefOutput.begin();
    size_t sizeAfterDeviceCall = dv_pair.first - koutput.begin();

    krefOutput.resize(sizeAfterCall);
    vrefOutput.resize(sizeAfterCall);
    koutput.resize(sizeAfterDeviceCall);
    voutput.resize(sizeAfterDeviceCall);

#if 0

    for(unsigned int i = 0; i < sizeAfterDeviceCall ; i++)
    {
        std::cout<<" -> OKeys "<<koutput[i].x<<" OValues "<<voutput[i].x<<std::endl;
    }

#endif

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);

}
TEST(ReduceByKeyPairUDDTest, MultiCoreUDDFloatIntTest)
{  
    int length = 10;
    std::vector< uddfltint > keys( length);
    uddfltint key;
    key.x = 1.0f;
    key.y = 1;
    std::vector< uddfltint > refInput( length );
    std::vector< uddfltint > input( length );

    for (int i = 0; i < length; i++)
    {
        if(std::rand()%5 == 1) key++;
        keys[i] = key;
        refInput[i].x = float(std::rand()%4);
        refInput[i].y = std::rand()%4;
        input[i] = refInput[i];
    }

    std::vector< uddfltint > koutput( length );
    std::vector< uddfltint > voutput( length );
    std::vector< uddfltint > krefOutput( length );
    std::vector< uddfltint > vrefOutput( length );

    // Instead of using fill
    krefOutput.clear();krefOutput.resize(length);
    vrefOutput.clear();vrefOutput.resize(length);
    voutput.clear();voutput.resize(length);
    koutput.clear();koutput.resize(length);

    uddfltint_equal_to binary_predictor;
    uddfltint_plus binary_operator;

    typedef std::pair<std::vector<uddfltint>::iterator,
            std::vector<uddfltint>::iterator> StdPairIterator;
    typedef bolt::cl::pair<std::vector<uddfltint>::iterator,
            std::vector<uddfltint>::iterator> DevicePairIterator;

    DevicePairIterator gold_pair =
    gold_reduce_by_key( keys.begin(),
                        keys.end(),
                        refInput.begin(),
                        krefOutput.begin(),
                        vrefOutput.begin(),
                        binary_operator);

    ::cl::Context myContext = bolt::cl::control::getDefault( ).getContext( );
    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    // call reduce_by_key
    DevicePairIterator dv_pair =
    bolt::cl::reduce_by_key(
        ctl,
        keys.begin(),
        keys.end(),
        input.begin(),
        koutput.begin(),
        voutput.begin(),
        binary_predictor,
        binary_operator);

    size_t sizeAfterCall = gold_pair.first - krefOutput.begin();
    size_t sizeAfterDeviceCall = dv_pair.first - koutput.begin();

    krefOutput.resize(sizeAfterCall);
    vrefOutput.resize(sizeAfterCall);
    koutput.resize(sizeAfterDeviceCall);
    voutput.resize(sizeAfterDeviceCall);

#if 0

    for(unsigned int i = 0; i < sizeAfterDeviceCall ; i++)
    {
        std::cout<<" -> OKeys "<<koutput[i].x<<" OValues "<<voutput[i].x<<std::endl;
    }

#endif

    cmpArrays(krefOutput, koutput);
    cmpArrays(vrefOutput, voutput);

}
#endif

TEST(reduce_by_key, differentKeys)
{
    int const size = 1<<10;
    std::vector<int> vectKeyIn(size);
    std::vector<int> vectValueIn(size,2);
    std::vector<int> keyBoltClDevVectOp(size);
    std::vector<int> valueBoltClDevVectOp(size);
    
    for (int i = 0; i < size; i++)
        vectKeyIn[i] = i+1;

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    bolt::cl::equal_to<int> eq;
    bolt::cl::reduce_by_key(ctl, vectKeyIn.begin(), vectKeyIn.end(), vectValueIn.begin(), keyBoltClDevVectOp.begin(),
                                                                                   valueBoltClDevVectOp.begin(), eq);
    static int eleKeyOp_Expexted[size];
    static int eleValueOp_Expexted[size];

    for (int i = 0; i < size; i++)
        eleKeyOp_Expexted[i] = i+1;

    for (int i = 0; i < size; i++)
        eleValueOp_Expexted[i] = 2;

    for (int i = 0; i < size; i++)
    {
        EXPECT_EQ ( eleKeyOp_Expexted[i], keyBoltClDevVectOp[i]);
        EXPECT_EQ ( eleValueOp_Expexted[i], valueBoltClDevVectOp[i]);
    }
}
TEST(reduce_by_key, differentKeysDeviceVector)
{
    int const size = 1<<10;
    bolt::cl::device_vector<int> vectKeyIn(size);
    bolt::cl::device_vector<int> vectValueIn(size,2);
    bolt::cl::device_vector<int> keyBoltClDevVectOp(size);
    bolt::cl::device_vector<int> valueBoltClDevVectOp(size);

    for (int i = 0; i < size; i++)
        vectKeyIn[i] = i+1;

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    bolt::cl::equal_to<int> eq;
    bolt::cl::reduce_by_key(ctl, vectKeyIn.begin(), vectKeyIn.end(), vectValueIn.begin(), keyBoltClDevVectOp.begin(),
                                                                                   valueBoltClDevVectOp.begin(), eq);
    static int eleKeyOp_Expexted[size];
    static int eleValueOp_Expexted[size];

    for (int i = 0; i < size; i++)
        eleKeyOp_Expexted[i] = i+1;

    for (int i = 0; i < size; i++)
        eleValueOp_Expexted[i] = 2;

    for (int i = 0; i < size; i++)
    {		
        EXPECT_EQ ( eleKeyOp_Expexted[i], keyBoltClDevVectOp[i]);
        EXPECT_EQ ( eleValueOp_Expexted[i], valueBoltClDevVectOp[i]);
    }

}

TEST(reduce_by_key, sameKeys)
{
    int const size = 1<<10;
    std::vector<int> vectKeyIn(size);
    std::vector<int> vectValueIn(size,2);
    std::vector<int> keyBoltClDevVectOp(size);
    std::vector<int> valueBoltClDevVectOp(size);
    
    for (int i = 0; i < size; i++)
        vectKeyIn[i] = 1;

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    bolt::cl::equal_to<int> eq;
    bolt::cl::reduce_by_key(ctl, vectKeyIn.begin(), vectKeyIn.end(), vectValueIn.begin(), keyBoltClDevVectOp.begin(),
                                                                                   valueBoltClDevVectOp.begin(), eq);
    static int eleKeyOp_Expexted[size];
    static int eleValueOp_Expexted[size];

    eleKeyOp_Expexted[0] = 1;
    eleValueOp_Expexted[0] = 2*size;
    
    for (int i = 0; i < size; i++)
    {
        EXPECT_EQ ( eleKeyOp_Expexted[i], keyBoltClDevVectOp[i]);
        EXPECT_EQ ( eleValueOp_Expexted[i], valueBoltClDevVectOp[i]);
    }

}
TEST(reduce_by_key, sameKeysDeviceVector)
{
    int const size = 1<<10;
    bolt::cl::device_vector<int> vectKeyIn(size);
    bolt::cl::device_vector<int> vectValueIn(size,2);
    bolt::cl::device_vector<int> keyBoltClDevVectOp(size);
    bolt::cl::device_vector<int> valueBoltClDevVectOp(size);

    for (int i = 0; i < size; i++)
        vectKeyIn[i] = 1;

    bolt::cl::control ctl = bolt::cl::control::getDefault();
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    bolt::cl::equal_to<int> eq;
    bolt::cl::reduce_by_key(ctl, vectKeyIn.begin(), vectKeyIn.end(), vectValueIn.begin(), keyBoltClDevVectOp.begin(),
                                                                                   valueBoltClDevVectOp.begin(), eq);
    static int eleKeyOp_Expexted[size];
    static int eleValueOp_Expexted[size];

    eleKeyOp_Expexted[0] = 1;
    eleValueOp_Expexted[0] = 2*size;
    
    for (int i = 0; i < size; i++)
    {
        EXPECT_EQ ( eleKeyOp_Expexted[i], keyBoltClDevVectOp[i]);
        EXPECT_EQ ( eleValueOp_Expexted[i], valueBoltClDevVectOp[i]);
    }

}

TEST(reduce_by_key, different1Keys)
{
    int const size = 1<<10;
    std::vector<int> vectKeyIn(size);
    std::vector<int> vectValueIn(size);
    std::vector<int> keyBoltClDevVectOp(size);
    std::vector<int> valueBoltClDevVectOp(size);

    for (int i = 0; i < size/4; i++)
        vectKeyIn[i] = 1;
    for (int i = size/4; i < size/2; i++)
        vectKeyIn[i] = 2;
    for (int i = size/2; i < (size*3)/4; i++)
        vectKeyIn[i] = 3;
    for (int i = (size*3)/4; i < size; i++)
        vectKeyIn[i] = 4;

    for (int i = 0; i < size; i++)
        vectValueIn[i] = 2; 

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    bolt::cl::equal_to<int> eq;
    bolt::cl::reduce_by_key(ctl, vectKeyIn.begin(), vectKeyIn.end(), vectValueIn.begin(), keyBoltClDevVectOp.begin(),
                                                                                valueBoltClDevVectOp.begin(), eq);
    static int eleKeyOp_Expexted[size];
    static int eleValueOp_Expexted[size];

    eleKeyOp_Expexted[0] = 1;
    eleKeyOp_Expexted[1] = 2;
    eleKeyOp_Expexted[2] = 3;
    eleKeyOp_Expexted[3] = 4;

    eleValueOp_Expexted[0] = 512;
    eleValueOp_Expexted[1] = 512;
    eleValueOp_Expexted[2] = 512;
    eleValueOp_Expexted[3] = 512;

    for (int i = 0; i < size; i++)
    {
        EXPECT_EQ ( eleKeyOp_Expexted[i], keyBoltClDevVectOp[i]);
        EXPECT_EQ ( eleValueOp_Expexted[i], valueBoltClDevVectOp[i]);
    }
}
TEST(reduce_by_key, different1KeysDeviceVector)
{
    int const size = 1<<10;
    bolt::cl::device_vector<int> vectKeyIn(size);
    bolt::cl::device_vector<int> vectValueIn(size,2);
    bolt::cl::device_vector<int> keyBoltClDevVectOp(size);
    bolt::cl::device_vector<int> valueBoltClDevVectOp(size);

    for (int i = 0; i < size/4; i++)
        vectKeyIn[i] = 1;
    for (int i = size/4; i < size/2; i++)
        vectKeyIn[i] = 2;
    for (int i = size/2; i < (size*3)/4; i++)
        vectKeyIn[i] = 3;
    for (int i = (size*3)/4; i < size; i++)
        vectKeyIn[i] = 4;

    bolt::cl::control ctl = bolt::cl::control::getDefault( );
    ctl.setForceRunMode(bolt::cl::control::MultiCoreCpu);

    bolt::cl::equal_to<int> eq;
    bolt::cl::reduce_by_key(ctl, vectKeyIn.begin(), vectKeyIn.end(), vectValueIn.begin(), keyBoltClDevVectOp.begin(),
                                                                                valueBoltClDevVectOp.begin(), eq);
    static int eleKeyOp_Expexted[size];
    static int eleValueOp_Expexted[size];

    eleKeyOp_Expexted[0] = 1;
    eleKeyOp_Expexted[1] = 2;
    eleKeyOp_Expexted[2] = 3;
    eleKeyOp_Expexted[3] = 4;

    eleValueOp_Expexted[0] = 512;
    eleValueOp_Expexted[1] = 512;
    eleValueOp_Expexted[2] = 512;
    eleValueOp_Expexted[3] = 512;

    for (int i = 0; i < size; i++)
    {
        EXPECT_EQ ( eleKeyOp_Expexted[i], keyBoltClDevVectOp[i]);
        EXPECT_EQ ( eleValueOp_Expexted[i], valueBoltClDevVectOp[i]);
    }
}

int _tmain(int argc, _TCHAR* argv[])
{
    //  Register our minidump generating logic
    bolt::miniDumpSingleton::enableMiniDumps( );

    //  Initialize googletest; this removes googletest specific flags from command line
    ::testing::InitGoogleTest( &argc, &argv[ 0 ] );

    bool print_clInfo = false;
    cl_uint userPlatform = 0;
    cl_uint userDevice = 0;
    cl_device_type deviceType = CL_DEVICE_TYPE_DEFAULT;

    try
    {
        // Declare supported options below, describe what they do
        boost::program_options::options_description desc( "Scan GoogleTest command line options" );
        desc.add_options()
            ( "help,h",         "produces this help message" )
            ( "queryOpenCL,q",  "Print queryable platform and device info and return" )
            ( "platform,p",     boost::program_options::value< cl_uint >( &userPlatform )->default_value( 0 ),
                                "Specify the platform under test" )
            ( "device,d",       boost::program_options::value< cl_uint >( &userDevice )->default_value( 0 ),
                               "Specify the device under test" )
            ;


        boost::program_options::variables_map vm;
        boost::program_options::store( boost::program_options::parse_command_line( argc, argv, desc ), vm );
        boost::program_options::notify( vm );

        if( vm.count( "help" ) )
        {
            // This needs to be 'cout' as program-options does not support wcout yet
            std::cout << desc << std::endl;
            return 0;
        }

        if( vm.count( "queryOpenCL" ) )
        {
            print_clInfo = true;
        }

        //  The following 3 options are not implemented yet; they are meant to be used with ::clCreateContextFromType()
        if( vm.count( "gpu" ) )
        {
            deviceType = CL_DEVICE_TYPE_GPU;
        }

        if( vm.count( "cpu" ) )
        {
            deviceType = CL_DEVICE_TYPE_CPU;
        }

        if( vm.count( "all" ) )
        {
            deviceType = CL_DEVICE_TYPE_ALL;
        }

    }
    catch( std::exception& e )
    {
        std::cout << _T( "Scan GoogleTest error condition reported:" ) << std::endl << e.what() << std::endl;
        return 1;
    }

    //  Query OpenCL for available platforms
    cl_int err = CL_SUCCESS;

    // Platform vector contains all available platforms on system
    std::vector< cl::Platform > platforms;
    //std::cout << "HelloCL!\nGetting Platform Information\n";
    bolt::cl::V_OPENCL( cl::Platform::get( &platforms ), "Platform::get() failed" );

    if( print_clInfo )
    {
        bolt::cl::control::printPlatforms( );
        return 0;
    }

    //  Do stuff with the platforms
    std::vector<cl::Platform>::iterator i;
    if(platforms.size() > 0)
    {
        for(i = platforms.begin(); i != platforms.end(); ++i)
        {
            if(!strcmp((*i).getInfo<CL_PLATFORM_VENDOR>(&err).c_str(), "Advanced Micro Devices, Inc."))
            {
                break;
            }
        }
    }
    bolt::cl::V_OPENCL( err, "Platform::getInfo() failed" );

    // Device info
    std::vector< cl::Device > devices;
    bolt::cl::V_OPENCL( platforms.front( ).getDevices( CL_DEVICE_TYPE_ALL, &devices ),"Platform::getDevices() failed");

    cl::Context myContext( devices.at( userDevice ) );
    cl::CommandQueue myQueue( myContext, devices.at( userDevice ) );
    bolt::cl::control::getDefault( ).setCommandQueue( myQueue );

    std::string strDeviceName = bolt::cl::control::getDefault( ).getDevice( ).getInfo< CL_DEVICE_NAME >( &err );
    bolt::cl::V_OPENCL( err, "Device::getInfo< CL_DEVICE_NAME > failed" );

    std::cout << "Device under test : " << strDeviceName << std::endl;

    int retVal = RUN_ALL_TESTS( );

    //  Reflection code to inspect how many tests failed in gTest
    ::testing::UnitTest& unitTest = *::testing::UnitTest::GetInstance( );

    unsigned int failedTests = 0;
    for( int i = 0; i < unitTest.total_test_case_count( ); ++i )
    {
        const ::testing::TestCase& testCase = *unitTest.GetTestCase( i );
        for( int j = 0; j < testCase.total_test_count( ); ++j )
        {
            const ::testing::TestInfo& testInfo = *testCase.GetTestInfo( j );
            if( testInfo.result( )->Failed( ) )
                ++failedTests;
        }
    }

    //  Print helpful message at termination if we detect errors, to help users figure out what to do next
    if( failedTests )
    {
        bolt::tout << _T( "\nFailed tests detected in test pass; please run test again with:" ) << std::endl;
        bolt::tout << _T( "\t--gtest_filter=<XXX> to select a specific failing test of interest" ) << std::endl;
        bolt::tout << _T( "\t--gtest_catch_exceptions=0 to generate minidump of failing test, or" ) << std::endl;
        bolt::tout << _T( "\t--gtest_break_on_failure to debug interactively with debugger" ) << std::endl;
        bolt::tout << _T( "\t    (only on googletest assertion failures, not SEH exceptions)" ) << std::endl;
    }

  return retVal;
}
#endif