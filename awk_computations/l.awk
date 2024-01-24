{
    trips[$1] += $5
}
END {
    for (t in trips)
        printf "%s;%.6f\n", t, trips[t]
}