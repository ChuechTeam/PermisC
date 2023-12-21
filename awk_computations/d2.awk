{
    drivers[$6] += $5
}
END {
    for (c in drivers)
        print c, drivers[c]
}