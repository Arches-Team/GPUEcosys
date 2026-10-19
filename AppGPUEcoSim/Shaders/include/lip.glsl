float getLIP(uint s, float h, ivec2 treePos, ivec2 offset)
{
	if (h < 4)
		return 0;
	ivec2 center = ivec2(N / 2);
	ivec2 coords = center + offset;

	// LIP discretized every 5m
	int patchIndex = int(min(h, 49.9) / 5.);

	coords.y += patchIndex * N;

	return imageLoad(lips, ivec3(coords, s)).r;
}

// Contribution of the tree standing at treePos to the LIF of the cell at treePos + offset,
// i.e. the exact factor compute_lif.glsl accumulated for that pair. Both the canopy cell
// (the tree's own, not the sampled one) and the patch offset matter: evaluating the patch
// at its center would remove the tree's peak shadow from every cell of its neighbourhood.
float GetLIFContribution(uint s, float h, ivec2 treePos, ivec2 offset)
{
    float c = imageLoad(canopy, treePos / 5).r;
    float scale = min(h / c, 1);// * sParams[s].ct;
    return 1 - scale * getLIP(s, h, treePos, offset);
}
