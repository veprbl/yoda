cdef class HistoBin1D(Bin1D_Dbn1D):

    cdef inline c.HistoBin1D *_HistoBin1D(self) except NULL:
        return <c.HistoBin1D *> self.ptr()

    def __init__(self, double a, double b):
        util.set_owned_ptr(
            self, new c.HistoBin1D(a, b))
        
    def fill(self, value=None, double weight=1.0):
        """
        (value=None, weight=1.0)

        Fill this bin with the given value and given weight. If value is None,
        fill at the midpoint of the bin.
        """
        try:
            self._HistoBin1D().fill(value, weight)
        except TypeError:
            self._HistoBin1D().fillBin(weight)

    @property
    def area(self):
        """
        b.area <==> b.sumW

        The area of the bin is the sum of weights of the bin; it is
        independent of width.

        """
        return self._HistoBin1D().area()

    @property
    def height(self):
        """
        b.height <==> b.area / b.width

        The height of the bin is defined as the area divided by the
        width.

        """
        return self._HistoBin1D().height()

    @property
    def areaErr(self):
        """
        Error computed using binomial statistics on squared sum of bin
        weights, i.e. s.areaErr = sqrt(s.sumW2)

        """
        return self._HistoBin1D().areaErr()

    @property
    def heightErr(self):
        """
        Height error - scales the s.areaError by the reciprocal of the
        bin width.

        """
        return self._HistoBin1D().heightErr()

    def __add__(HistoBin1D a, HistoBin1D b):
        return util.new_owned_cls(
            HistoBin1D,
            new c.HistoBin1D(
                deref(a._HistoBin1D()) + 
                deref(b._HistoBin1D())))

    def __sub__(HistoBin1D a, HistoBin1D b):
        return util.new_owned_cls(
            HistoBin1D,
            new c.HistoBin1D(
                deref(a._HistoBin1D()) -
                deref(b._HistoBin1D())))
