{
    drivers[$6] += $5
}
END {
    for (c in drivers)
        printf "%s;%.6f\n", c, drivers[c]
    # print c ";" drivers[c]
}