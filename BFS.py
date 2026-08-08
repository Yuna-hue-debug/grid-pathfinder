from collections import deque
grid = [
    ['A', 0,  0, '#', 0],
    [0, '#', 0, '#', 0],
    [0, 0,  0,  0, 0],
    ['#',0, '#', 'G',0],
    [0, 0,  0,  0, 0]
]
queue=deque()
r=0
c=0
start=(r,c)
visited=set()
queue.append(start)
visited.add(start)
directions=[
    (-1,0), # 上
    (1,0),  # 下
    (0,-1), # 左
    (0,1)   # 右
]
distance={}
distance[start]=0
while queue:
    r,c=queue.popleft()
    if grid[r][c]=='G':
        print('找到金币')
        print('距离：',distance[(r,c)])
        break
    for dr,dc in directions:
        nr=r+dr
        nc=c+dc
        print('尝试：',nr,nc)
        if nr>=0 and nr<len(grid) and nc>=0 and nc<len(grid[0]) and grid[nr][nc]!='#' and (nr,nc) not in visited:
            queue.append((nr,nc))
            visited.add((nr,nc))
            distance[(nr,nc)]=distance[(r,c)]+1
