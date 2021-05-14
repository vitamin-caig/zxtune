package app.zxtune.fs

object DefaultComparator : Comparator<VfsObject> {

    override fun compare(lh: VfsObject, rh: VfsObject) = compare(lh.name, rh.name)

    //allow access for test
    @JvmStatic
    fun compare(lh: String, rh: String) = compareAlphaNumeric(lh, rh)

    @JvmStatic
    fun instance() = this
}

private fun compareAlphaNumeric(lh: String, rh: String): Int {
    var compareResult = 0
    var caseSensitiveCompareResult = 0
    val lhSize = lh.length
    val rhSize = rh.length
    for (pos in 0 until minOf(lhSize, rhSize)) {
        val lhSym = lh[pos]
        val rhSym = rh[pos]

        if (isDigit(lhSym) && isDigit(rhSym)) {
            return when {
                compareResult != 0 -> compareResult
                caseSensitiveCompareResult != 0 -> caseSensitiveCompareResult
                else -> compareNumericPrefix(lh.substring(pos), rh.substring(pos))
            }
        } else if (lhSym != rhSym && compareResult == 0) {
            compareResult = compare(lhSym.uppercaseChar().code, rhSym.uppercaseChar().code)
            if (compareResult == 0 && caseSensitiveCompareResult == 0) {
                caseSensitiveCompareResult = compare(lhSym.code, rhSym.code)
            }
        }
    }
    return when {
        compareResult != 0 -> compareResult
        lhSize == rhSize -> caseSensitiveCompareResult
        lhSize < rhSize -> -1
        else -> 1
    }
}

private fun compareNumericPrefix(lh: String, rh: String): Int {
    val lhZeroes = countLeadingZeroes(lh)
    val rhZeroes = countLeadingZeroes(rh)
    return if (lhZeroes == 0 && rhZeroes == 0) {
        compareNumericNoZeroPrefix(lh, rh)
    } else {
        val suffixCompareResult =
            compareNumericNoZeroPrefix(lh.substring(lhZeroes), rh.substring(rhZeroes))
        if (suffixCompareResult != 0) {
            suffixCompareResult
        } else {
            -compare(lhZeroes, rhZeroes)
        }
    }
}

private fun countLeadingZeroes(str: String) = str.indexOfFirst { it != '0' }
    .let { if (it >= 0) it else str.length }

private fun compareNumericNoZeroPrefix(lh: String, rh: String): Int {
    var compareResult = 0
    for (pos in 0 until minOf(lh.length, rh.length)) {
        val lhSym = lh[pos]
        val rhSym = rh[pos]
        val lhDigit = isDigit(lhSym)
        val rhDigit = isDigit(rhSym)
        if (lhDigit && rhDigit) {
            if (compareResult == 0) {
                compareResult = compare(lhSym.code, rhSym.code)
            }
        } else return when {
            rhDigit -> -1
            lhDigit -> 1
            compareResult != 0 -> compareResult
            else -> compareAlphaNumeric(lh.substring(pos), rh.substring(pos))
        }
    }
    return when (val byLen = compare(lh.length, rh.length)) {
        0 -> compareResult
        else -> byLen
    }
}

private fun isDigit(c: Char) = c in '0'..'9'

private fun compare(lh: Int, rh: Int) = lh.compareTo(rh)
