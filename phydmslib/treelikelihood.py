"""Tree likelihoods.

Nucleotides, amino acids, and codons are indexed by integers 0, 1, ...
using the indexing schemes defined in `phydmslib.constants`.
"""


import scipy
import Bio.Phylo
import phydmslib.models
from phydmslib.constants import *


class TreeLikelihood:
    """Uses alignment, model, and tree to calculate likelihoods.

    See `__init__` for how to initialize a `TreeLikelihood`.

    A `TreeLikelihood` object is designed to only work for a fixed
    tree topology. In the internal workings, this fixed topology is
    transformed into the node indexing scheme defined by `name_to_nodeindex`.

    After initialization, most attributes should **not** be changed directly.
    They should only be altered via the `updateParams` method or
    by altering the `paramsarray` property. Both of these methods
    of updating attributes correctly update all of the dependent
    properties; setting other attributes directly does not.

    Attributes:
        `tree` (instance of `Bio.Phylo.BaseTree.Tree` derived class)
            Phylogenetic tree. 
        `model` (instance of `phydmslib.models.Model` derived class)
            Specifies the substitution model for `nsites` codon sites.
        `alignment` (list of 2-tuples of strings, `(head, seq)`)
            Aligned protein-coding codon sequences. Headers match
            tip names in `tree`; sequences contain `nsites` codons.
        `paramsarray` (`numpy.ndarray` of floats, 1-dimensional)
            An array of all of the free parameters. You can directly
            assign to `paramsarray`, and the correct parameters will
            be internally updated vi `updateParams`. 
            `paramsarray` is actually a property rather than an attribute.
        `loglik` (`float`)
            Current log likelihood.
        `siteloglik` (`numpy.ndarray` of floats, length `nsites`)
            `siteloglik[r]` is current log likelihood at site `r`.
        `dloglik` (`dict`)
            For each `param` in `model.freeparams`, `dloglik[param]`
            is the derivative of `loglik` with respect to `param`.
        `dloglikarray` (`numpy.ndarray` of floats, 1-dimensional)
            `dloglikarray[i]` is the derivative of `loglik` with respect
            to the parameter represented in `paramsarray[i]`. This is
            the same information as in `dloglik`, but in a different
            representation.
        `dsiteloglik` (`dict`)
            For each `param` in `model.freeparams`, `dsiteloglik[param][r]`
            is the derivative of `siteloglik[r]` with respect to to `param`.
        `nsites` (int)
            Number of codon sites.
        `nseqs` (int)
            Number of sequences in `alignment`, and tip nodes in `tree`.
        `nnodes` (int)
            Total number of nodes in `tree`, counting tip and internal.
            Node are indexed 0, 1, ..., `nnodes - 1`. This indexing is done
            such that the descendants of node `n` always have indices < `n`.
        `name_to_nodeindex` (`dict`)
            For each sequence name (these are headers in `alignmnent`, 
            tip names in `tree`), `name_to_nodeindex[name]` is the `int`
            index of that `node` in the internal indexing scheme.
        `internalnodes` (`list` of `int`, length `nnodes`)
            List of the indices of internal nodes only. Given the node 
            indexing scheme, the root node will always be the last one
            in this list.
        `rdescend` and `ldescend` (each a `list` of `int` of length `nnode`)
            For each node `n`, if `n` is an internal node then `rdescend[n]`
            is its right descendant and `ldescend[n]` is its left descendant.
            If `n` is a tip node, then `rdescend[n] == ldescend[n] == -1`.
            Note that the indexing scheme ensures that `rdescend[n] < n`
            and `ldescend[n] < n` for all `n` in `internalnodes`.
        `t` (`list` of `float`, length `nnodes - 1`)
            `t[n]` is the branch length leading node `n`. The branch leading
            to the root node (which has index `nnodes - 1`) is undefined
            and not used, which is why `t` is of length `nnodes - 1` rather
            than `nnodes`.
        `L` (`numpy.ndarray` of `float`, shape `(nnodes, nsites, N_CODON)`)
            `L[n][r][x]` is the partial conditional likelihood of codon 
            `x` at site `r` at node `n`.
        `dL` (`dict` keyed by strings, values `numpy.ndarray` of `float`)
            For each free model parameter `param` in `model.freeparam`, 
            `dL[param]` is derivative of `L` with respect to `param`.
            If `param` is a float, then `dL[param][n][r][x]` is derivative
            of `L[n][r][x]` with respect to `param`. If `param` is an array,
            then `dL[param][n][i][r][x]` is derivative of `L[n][r][x]` 
            with respect to `param[i]`.
    """

    def __init__(self, tree, alignment, model):
        """Initialize a `TreeLikelihood` object.

        Args:
            `tree`, `model`, `alignment`
                Attributes of same name described in class doc string.
        """
        assert isinstance(model, phydmslib.models.Model), "invalid model"
        self.model = model
        self.nsites = self.model.nsites

        assert isinstance(alignment, list) and all([isinstance(tup, tuple)
                and len(tup) == 2 for tup in alignment]), ("alignment is "
                "not list of (head, seq) 2-tuples")
        assert all([len(seq) == 3 * self.nsites for (head, seq) in alignment])
        assert set([head for (head, seq) in alignment]) == set([clade.name for
                clade in tree.get_terminals()])
        self.alignment = alignment
        self.nseqs = len(alignment)
        alignment_d = dict(self.alignment)

        assert isinstance(tree, Bio.Phylo.BaseTree.Tree), "invalid tree"
        self.tree = tree
        assert self.tree.count_terminals() == self.nseqs

        # index nodes
        self.name_to_nodeindex = {}
        self.nnodes = tree.count_terminals() + len(tree.get_nonterminals())
        self.rdescend = [-1] * self.nnodes
        self.ldescend = [-1] * self.nnodes
        self.t = [-1] * (self.nnodes - 1)
        self.internalnodes = [] 
        self.L = scipy.full((self.nnodes, self.nsites, N_CODON), -1, dtype='float')
        self.dL = {}
        for param in self.model.freeparams:
            paramvalue = getattr(self.model, param)
            if isinstance(paramvalue, float):
                self.dL[param] = scipy.full((self.nnodes, self.nsites, N_CODON), 
                        -1, dtype='float')
            elif isinstance(paramvalue, scipy.ndarray) and (paramvalue.shape
                    == (len(paramvalue),)):
                self.dL[param] = scipy.full((self.nnodes, len(paramvalue),
                        self.nsites, N_CODON), -1, dtype='float')
            else:
                raise ValueError("Cannot handle param: {0}, {1}".format(
                        param, paramvalue))
        for (n, node) in enumerate(self.tree.find_clades(order='postorder')):
            if node != self.tree.root:
                assert n < self.nnodes - 1
                self.t[n] = node.branch_length
            if node.is_terminal():
                seq = alignment_d[node.name]
                for r in range(self.nsites):
                    codon = seq[3 * r : 3 * r + 3]
                    if codon == '---':
                        scipy.copyto(self.L[n][r], scipy.ones(N_CODON, 
                                dtype='float'))
                    elif codon in CODON_TO_INDEX:
                        scipy.copyto(self.L[n][r], scipy.zeros(N_CODON, 
                                dtype='float'))
                        self.L[n][r][CODON_TO_INDEX[codon]] = 1.0
                    else:
                        raise ValueError("Bad codon {0} in {1}".format(codon, 
                                node.name))
                for param in self.model.freeparams:
                    paramvalue = getattr(self.model, param)
                    if isinstance(paramvalue, float):
                        self.dL[param][n].fill(0.0)
                    else:
                        for i in range(len(paramvalue)):
                            self.dL[param][n][i].fill(0.0)
            else:
                assert len(node.clades) == 2, "not 2 children: {0}".format(node.name)
                self.rdescend[n] = self.name_to_nodeindex[node.clades[0]]
                self.ldescend[n] = self.name_to_nodeindex[node.clades[1]]
                self.internalnodes.append(n)
            self.name_to_nodeindex[node] = n
        assert len(self.internalnodes) == len(tree.get_nonterminals())

        # _index_to_param defines internal mapping of
        # `paramsarray` indices and parameters
        self._index_to_param = {} # value is parameter associated with index
        i = 0
        for param in self.model.freeparams:
            paramvalue = getattr(self.model, param)
            if isinstance(paramvalue, float):
                self._index_to_param[i] = param
                i += 1
            else:
                for (pindex, pvalue) in enumerate(paramvalue):
                    self._index_to_param[i] = (param, pindex)
                    i += 1
        self._param_to_index = dict([(y, x) for (x, y) in 
                self._index_to_param.items()])
        assert i == len(self._index_to_param) 

        # now update internal attributes related to likelihood
        self._updateInternals()

    @property
    def paramsarray(self):
        """All free model parameters as 1-dimensional `numpy.ndarray`.
        
        You are allowed to update model parameters by direct
        assignment of this property."""
        nparams = len(self._index_to_param)
        paramsarray = scipy.ndarray(shape=(nparams,), dtype='float')
        for (i, param) in self._index_to_param.items():
            if isinstance(param, str):
                paramsarray[i] = getattr(self.model, param)
            elif isinstance(param, tuple):
                paramsarray[i] = getattr(self.model, param[0])[param[1]]
        return paramsarray

    @paramsarray.setter
    def paramsarray(self, value):
        """Set new `paramsarray` and update via `updateParams`."""
        nparams = len(self._index_to_param)
        assert (isinstance(value, scipy.ndarray) and len(value.shape) == 1), (
                "paramsarray must be 1-dim ndarray")
        assert len(value) == nparams, ("Assigning paramsarray to ndarray "
                "of the wrong length.")
        # build `newvalues` to pass to `updateParams`
        newvalues = {}
        vectorized_params = {}
        for (i, param) in self._index_to_param.items():
            if isinstance(param, str):
                newvalues[param] = float(value[i])
            elif isinstance(param, tuple):
                (iparam, iparamindex) = param
                if iparam in vectorized_params:
                    assert iparamindex not in vectorized_params[iparam]
                    vectorized_params[iparam][iparamindex] = float(value[i])
                else:
                    vectorized_params[iparam] = {iparamindex:float(value[i])}
        for (param, paramd) in vectorized_params.items():
            assert set(paramd.keys()) == set(range(len(paramd)))
            newvalues[param] = scipy.array([paramd[i] for i in range(len(paramd))],
                    dtype='float')
        self.updateParams(newvalues)

    @property
    def dloglikarray(self):
        """Derivative of `loglik` with respect to `paramsarray`."""
        nparams = len(self._index_to_param)
        dloglikarray = scipy.ndarray(shape=(nparams,), dtype='float')
        for (i, param) in self._index_to_param.items():
            if isinstance(param, str):
                dloglikarray[i] = self.dloglik[param]
            elif isinstance(param, tuple):
                dloglikarray[i] = self.dloglik[param[0]][param[1]]
        return dloglikarray

    def updateParams(self, newvalues):
        """Update parameters and re-compute likelihoods.

        This method is the **only** acceptable way to update model
        or tree parameters. The likelihood is re-computed as needed
        by this method.

        Args:
            `newvalues` (dict)
                A dictionary keyed by param name and with value as new
                value to set. Each parameter name must either be
                a string in `model.freeparams` or a string that represents
                a valid branch length.
        """
        modelparams = {}
        otherparams = {}
        for (param, value) in newvalues.items():
            if param in self.model.freeparams:
                modelparams[param] = value
            else:
                otherparms[param] = value
        if modelparams:
            self.model.updateParams(modelparams)
        if otherparams:
            raise RuntimeError("cannot currently handle non-model params")
        if newvalues:
            self._updateInternals()

    def _updateInternals(self):
        """Update internal attributes related to likelihood.

        Should be called any time branch lengths or model parameters
        are changed.
        """
        with scipy.errstate(over='raise', under='raise', divide='raise',
                invalid='raise'):
            self._computePartialLikelihoods()
            self.siteloglik = scipy.log(scipy.sum(self.L[-1] * 
                    self.model.stationarystate, axis=1))
            self.loglik = scipy.sum(self.siteloglik)
            self.dsiteloglik = {}
            self.dloglik = {}
            for param in self.model.freeparams:
                self.dsiteloglik[param] = scipy.sum(self.model.dstationarystate(param)
                        * self.L[-1] + self.dL[param][-1] 
                        * self.model.stationarystate, axis=-1) / self.siteloglik
                self.dloglik[param] = scipy.sum(self.dsiteloglik[param], axis=-1)

    def _computePartialLikelihoods(self):
        """Update `L`."""
        for n in self.internalnodes:
            nright = self.rdescend[n]
            nleft = self.ldescend[n]
            tright = self.t[nright]
            tleft = self.t[nleft]
            Mright = self.model.M(tright)
            Mleft = self.model.M(tleft)
            # using this approach to broadcast matrix-vector mutiplication
            # http://stackoverflow.com/questions/26849910/numpy-matrix-multiplication-broadcast
            MLright = scipy.sum(Mright * self.L[nright][:, None, :], axis=2)
            MLleft = scipy.sum(Mleft * self.L[nleft][:, None, :], axis=2)
            scipy.copyto(self.L[n], MLright * MLleft)
            for param in self.model.freeparams:
                paramvalue = getattr(self.model, param)
                if isinstance(paramvalue, float):
                    dMLright = scipy.sum(self.model.dM(tright, param) * 
                            self.L[nright][:, None, :], axis=2)
                    MdLright = scipy.sum(Mright * self.dL[param][nright][:,
                            None, :], axis=2)
                    dMLleft = scipy.sum(self.model.dM(tleft, param) * 
                            self.L[nleft][:, None, :], axis=2)
                    MdLleft = scipy.sum(Mleft * self.dL[param][nleft][:, None, :], 
                            axis=2)
                    scipy.copyto(self.dL[param][n], (dMLright + MdLright) * MLleft
                            + MLright * (dMLleft + MdLleft))
                else:
                    for i in range(len(paramvalue)):
                        dMLright = scipy.sum(self.model.dM(tright, param)[i] * 
                                self.L[nright][:, None, :], axis=2)
                        MdLright = scipy.sum(Mright * self.dL[param][nright][i][:, 
                                None, :], axis=2)
                        dMLleft = scipy.sum(self.model.dM(tleft, param)[i] * 
                                self.L[nleft][:, None, :], axis=2)
                        MdLleft = scipy.sum(Mleft * self.dL[param][nleft][i][:, 
                                None, :], axis=2)
                        scipy.copyto(self.dL[param][n][i], (dMLright + MdLright)
                                * MLleft + MLright * (dMLleft + MdLleft))


if __name__ == '__main__':
    import doctest
    doctest.testmod()
