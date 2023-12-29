{
    if (!seen[$6,$1]++)
        count[$6]++
}
END {
    for (driver in count)
        print driver ";" count[driver]
}