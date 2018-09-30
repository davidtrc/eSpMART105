package com.lazaro.b105.valorizab105.fragments;

import java.util.Comparator;
import java.util.Date;

public class ArrayIndexComparator implements Comparator<Integer>
{
    private final Date[] array;

    public ArrayIndexComparator(Date[] array)
    {
        this.array = array;
    }

    public Integer[] createIndexArray()
    {
        Integer[] indexes = new Integer[array.length];
        for (int i = 0; i < array.length; i++)
        {
            indexes[i] = i; // Autoboxing
        }
        return indexes;
    }

    @Override
    public int compare(Integer index1, Integer index2)
    {
        // Autounbox from Integer to int to use as array indexes
        return -(array[index1].compareTo(array[index2]));
    }
}
