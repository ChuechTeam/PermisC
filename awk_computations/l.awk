{
    trips[$1] += $5
}
END {
    for (t in trips)
        print t, trips[t]
}